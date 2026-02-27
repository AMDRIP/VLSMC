#include "kernel/ahci.h"
#include "kernel/pci.h"
#include "kernel/vmm.h"
#include "kernel/pmm.h"
#include "kernel/timer.h"
#include "libc.h"

namespace re36 {

HBA_MEM* AHCIDriver::abarz = nullptr;
static int s_primary_port = -1;

static inline void mfence() {
    asm volatile("mfence" ::: "memory");
}

PCIDevice* AHCIDriver::find_ahci_controller() {
    int count = PCI::get_device_count();
    PCIDevice* devs = PCI::get_devices();
    
    for (int i = 0; i < count; i++) {
        if (devs[i].class_id == 0x01 && devs[i].subclass == 0x06 && devs[i].prog_if == 0x01) {
            return &devs[i];
        }
    }
    return nullptr;
}

void AHCIDriver::init() {
    PCIDevice* dev = find_ahci_controller();
    if (!dev) {
        printf("[AHCI] No controller found on PCI bus.\n");
        return;
    }

    printf("[AHCI] Found controller at %d:%d:%d\n", dev->bus, dev->slot, dev->func);
    
    uint32_t bar5_val = PCI::config_read_dword(dev->bus, dev->slot, dev->func, 0x24);
    if (bar5_val & 0x01) { // It's I/O space
        printf("[AHCI] BAR5 is I/O space, expecting Memory space. Abort.\n");
        return;
    }

    uint32_t ahci_base = bar5_val & 0xFFFFFFF0;
    
    // Check if 64-bit BAR
    if ((bar5_val & 0x06) == 0x04) {
        uint32_t bar6_val = PCI::config_read_dword(dev->bus, dev->slot, dev->func, 0x28);
        // We only support 32-bit physical addresses here in our 32-bit OS usually
        if (bar6_val != 0) {
            printf("[AHCI] Warning: 64-bit BAR5 mapped above 4GB, truncating.\n");
        }
    }

    // Enable bus master
    uint16_t cmd = PCI::config_read_word(dev->bus, dev->slot, dev->func, 0x04);
    PCI::config_write_word(dev->bus, dev->slot, dev->func, 0x04, cmd | 0x04 | 0x02);

    abarz = (HBA_MEM*)ahci_base;

    // Identity map the AHCI memory space if VMM is active.
    // AHCI spec says minimum size is 4KB. Let's map 8KB to be safe for 32 ports.
    VMM::map_page(ahci_base, ahci_base, PAGE_PRESENT | PAGE_WRITABLE | PAGE_CACHEDISABLE);
    VMM::map_page(ahci_base + 4096, ahci_base + 4096, PAGE_PRESENT | PAGE_WRITABLE | PAGE_CACHEDISABLE);

    // Global Host Control (AE - AHCI Enable)
    abarz->ghc |= (uint32_t)(1 << 31);
    
    // Setup ports
    uint32_t pi = abarz->pi;
    for (int i = 0; i < 32; i++) {
        if (pi & 1) {
            check_port(&abarz->ports[i], i);
        }
        pi >>= 1;
    }
}

void AHCIDriver::check_port(HBA_PORT* port, int port_no) {
    uint32_t ssts = port->ssts;

    uint8_t ipm = (ssts >> 8) & 0x0F;
    uint8_t det = ssts & 0x0F;

    if (det != HBA_PORT_DET_PRESENT || ipm != HBA_PORT_IPM_ACTIVE) {
        return; // Device not present or not active
    }

    // Check device type via signature
    uint32_t sig = port->sig;
    int dev_type = AHCI_DEV_NULL;

    if (sig == 0x00000101) dev_type = AHCI_DEV_SATA;
    else if (sig == 0xEB140101) dev_type = AHCI_DEV_SATAPI;
    else if (sig == 0xC33C0101) dev_type = AHCI_DEV_SEMB;
    else if (sig == 0x96690101) dev_type = AHCI_DEV_PM;

    if (dev_type == AHCI_DEV_SATA) {
        printf("[AHCI] SATA drive found on port %d\n", port_no);
        port_rebase(port, port_no);
    } else if (dev_type == AHCI_DEV_SATAPI) {
        printf("[AHCI] SATAPI drive found on port %d\n", port_no);
    }
}

void AHCIDriver::start_cmd(HBA_PORT* port) {
    while (port->cmd & (1 << 15)); // Wait until CR (command list running) is clear
    port->cmd |= (1 << 4);  // FRE = 1
    port->cmd |= (1 << 0);  // ST  = 1
    mfence();
}

void AHCIDriver::stop_cmd(HBA_PORT* port) {
    port->cmd &= ~(1 << 0); // ST = 0
    port->cmd &= ~(1 << 4); // FRE = 0
    
    // Wait until FR and CR are clear
    while (1) {
        if (port->cmd & (1 << 14)) continue; // FR
        if (port->cmd & (1 << 15)) continue; // CR
        break;
    }
}

void AHCIDriver::port_rebase(HBA_PORT* port, int port_no) {
    if (s_primary_port == -1) {
        s_primary_port = port_no;
    }

    stop_cmd(port);

    // Command list offset: 1K, FIS offset: 256 bytes. Give them 1 frame (4KB) per port.
    void* frame = PhysicalMemoryManager::alloc_frame();
    if (!frame) {
        printf("[AHCI] OOM allocating structures for port %d\n", port_no);
        return;
    }
    
    // Zero memory
    uint8_t* mem = (uint8_t*)frame;
    for (int i = 0; i < 4096; i++) mem[i] = 0;

    port->clb = (uint32_t)frame;
    port->clbu = 0;
    
    port->fb = (uint32_t)frame + 1024; // FIS base at offset 1024
    port->fbu = 0;

    // Command Tables. We have 32 command headers. 
    // Each CT takes around 256 bytes for a few PRDT entries. 
    // 32 CTs * 256 bytes = 8192 bytes = 2 frames.
    HBA_CMD_HEADER* cmdheader = (HBA_CMD_HEADER*)port->clb;
    uint32_t ct_phys = 0;

    for (int i = 0; i < 32; i++) {
        cmdheader[i].prdtl = 8;

        if (i % 16 == 0) {
            void* f = PhysicalMemoryManager::alloc_frame();
            if (!f) return;
            ct_phys = (uint32_t)f;
            uint8_t* p = (uint8_t*)f;
            for (int k = 0; k < 4096; k++) p[k] = 0;
        }

        uint32_t ct_addr = ct_phys + (i % 16) * 256;
        cmdheader[i].ctba = ct_addr;
        cmdheader[i].ctbau = 0;
    }

    start_cmd(port);
}

int AHCIDriver::find_cmdslot(HBA_PORT* port) {
    // If not set in SACT and CI, the slot is free
    uint32_t slots = port->sact | port->ci;
    for (int i = 0; i < 32; i++) {
        if ((slots & 1) == 0)
            return i;
        slots >>= 1;
    }
    printf("[AHCI] Cannot find free command list slot\n");
    return -1;
}

bool AHCIDriver::read(uint8_t port_no, uint64_t lba, uint32_t sector_count, void* buffer_phys) {
    if (abarz == nullptr) return false;
    port_no &= 0x1F;
    HBA_PORT* port = &abarz->ports[port_no];

    port->is = (uint32_t)-1; // Clear pending interrupts

    int slot = find_cmdslot(port);
    if (slot == -1) return false;

    HBA_CMD_HEADER* cmdheader = (HBA_CMD_HEADER*)port->clb;
    cmdheader += slot;
    
    cmdheader->cfl = sizeof(FIS_REG_H2D) / sizeof(uint32_t); // 5 dwords
    cmdheader->a = 0;
    cmdheader->w = 0; // Read from device
    cmdheader->prdtl = 1;

    HBA_CMD_TBL* cmdtbl = (HBA_CMD_TBL*)(cmdheader->ctba);
    uint8_t* cmdbuf = (uint8_t*)cmdtbl;
    for (int i = 0; i < 256; i++) cmdbuf[i] = 0;

    // Single PRDT entry
    cmdtbl->prdt_entry[0].dba = (uint32_t)buffer_phys;
    cmdtbl->prdt_entry[0].dbau = 0;
    cmdtbl->prdt_entry[0].dbc = (sector_count * 512) - 1; // bytes - 1
    cmdtbl->prdt_entry[0].i = 1; // Interrupt when done

    // Setup FIS
    FIS_REG_H2D *cmdfis = (FIS_REG_H2D*)(&cmdtbl->cfis);
    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1;  // Command
    cmdfis->command = 0x24; // READ DMA EXT (LBA48)
    
    cmdfis->lba0 = (uint8_t)lba;
    cmdfis->lba1 = (uint8_t)(lba >> 8);
    cmdfis->lba2 = (uint8_t)(lba >> 16);
    cmdfis->device = 0x40; // LBA mode
    
    cmdfis->lba3 = (uint8_t)(lba >> 24);
    cmdfis->lba4 = (uint8_t)(lba >> 32);
    cmdfis->lba5 = (uint8_t)(lba >> 40);
    
    cmdfis->countl = sector_count & 0xFF;
    cmdfis->counth = (sector_count >> 8) & 0xFF;

    // Wait until port is no longer busy
    int spin = 0;
    while ((port->tfd & (0x80 | 0x08)) && spin < 1000000) { spin++; }
    if (spin == 1000000) {
        printf("[AHCI] Port is hung\n");
        return false;
    }

    port->ci = 1 << slot; // Issue command
    mfence();
    
    // Wait for completion
    while (1) {
        if ((port->ci & (1 << slot)) == 0) 
            break;
        if (port->is & (1 << 30)) { // Task file error
            printf("[AHCI] Read disk error\n");
            return false;
        }
    }

    if (port->tfd & 0x01) { // Error bit
        printf("[AHCI] Task file read error\n");
        return false;
    }

    return true;
}

bool AHCIDriver::write(uint8_t port_no, uint64_t lba, uint32_t sector_count, void* buffer_phys) {
    if (abarz == nullptr) return false;
    port_no &= 0x1F;
    HBA_PORT* port = &abarz->ports[port_no];

    port->is = (uint32_t)-1; // Clear pending ints

    int slot = find_cmdslot(port);
    if (slot == -1) return false;

    HBA_CMD_HEADER* cmdheader = (HBA_CMD_HEADER*)port->clb;
    cmdheader += slot;
    
    cmdheader->cfl = sizeof(FIS_REG_H2D) / sizeof(uint32_t);
    cmdheader->a = 0;
    cmdheader->w = 1; // Write to device
    cmdheader->prdtl = 1;

    HBA_CMD_TBL* cmdtbl = (HBA_CMD_TBL*)(cmdheader->ctba);
    uint8_t* cmdbuf = (uint8_t*)cmdtbl;
    for (int i = 0; i < 256; i++) cmdbuf[i] = 0;

    cmdtbl->prdt_entry[0].dba = (uint32_t)buffer_phys;
    cmdtbl->prdt_entry[0].dbau = 0;
    cmdtbl->prdt_entry[0].dbc = (sector_count * 512) - 1;
    cmdtbl->prdt_entry[0].i = 1;

    FIS_REG_H2D *cmdfis = (FIS_REG_H2D*)(&cmdtbl->cfis);
    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1;
    cmdfis->command = 0x35; // WRITE DMA EXT
    
    cmdfis->lba0 = (uint8_t)lba;
    cmdfis->lba1 = (uint8_t)(lba >> 8);
    cmdfis->lba2 = (uint8_t)(lba >> 16);
    cmdfis->device = 0x40; // LBA mode
    
    cmdfis->lba3 = (uint8_t)(lba >> 24);
    cmdfis->lba4 = (uint8_t)(lba >> 32);
    cmdfis->lba5 = (uint8_t)(lba >> 40);
    
    cmdfis->countl = sector_count & 0xFF;
    cmdfis->counth = (sector_count >> 8) & 0xFF;

    int spin = 0;
    while ((port->tfd & (0x80 | 0x08)) && spin < 1000000) { spin++; }
    if (spin == 1000000) {
        printf("[AHCI] Port is hung\n");
        return false;
    }

    port->ci = 1 << slot;
    mfence();
    
    while (1) {
        if ((port->ci & (1 << slot)) == 0) 
            break;
        if (port->is & (1 << 30)) { 
            printf("[AHCI] Write disk error\n");
            return false;
        }
    }

    if (port->tfd & 0x01) { 
        printf("[AHCI] Task file write error\n");
        return false;
    }

    return true;
}

bool AHCIDriver::is_present() {
    return s_primary_port != -1;
}

int AHCIDriver::get_primary_port() {
    return s_primary_port;
}

} // namespace re36
