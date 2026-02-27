#include "kernel/pci.h"
#include "kernel/pic.h"
#include "kernel/spinlock.h"
#include "libc.h"

namespace re36 {

static inline void outportl(uint16_t port, uint32_t data) {
    asm volatile("outl %0, %1" : : "a"(data), "Nd"(port));
}

static inline uint32_t inportl(uint16_t port) {
    uint32_t result;
    asm volatile("inl %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

static PCIDevice s_devices[32];
static int s_device_count = 0;

void PCI::init() {
    s_device_count = 0;
}

uint32_t PCI::config_read_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)(bus & 0xFF);
    uint32_t lslot = (uint32_t)(slot & 0x1F);
    uint32_t lfunc = (uint32_t)(func & 0x07);
    uint32_t loffset = (uint32_t)(offset & 0xFC);
 
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | loffset | ((uint32_t)0x80000000));
 
    InterruptGuard guard;
    outportl(PCI_CONFIG_ADDRESS, address);
    return inportl(PCI_CONFIG_DATA);
}

uint16_t PCI::config_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t dword_val = config_read_dword(bus, slot, func, offset);
    return (uint16_t)((dword_val >> ((offset & 2) * 8)) & 0xFFFF);
}

uint8_t PCI::config_read_byte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t dword_val = config_read_dword(bus, slot, func, offset);
    return (uint8_t)((dword_val >> ((offset & 3) * 8)) & 0xFF);
}

void PCI::config_write_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)(bus & 0xFF);
    uint32_t lslot = (uint32_t)(slot & 0x1F);
    uint32_t lfunc = (uint32_t)(func & 0x07);
    uint32_t loffset = (uint32_t)(offset & 0xFC);

    address = (uint32_t)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | loffset | ((uint32_t)0x80000000));
              
    InterruptGuard guard;
    outportl(PCI_CONFIG_ADDRESS, address);
    outportl(PCI_CONFIG_DATA, value);
}

void PCI::config_write_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t value) {
    uint32_t dword_val = config_read_dword(bus, slot, func, offset);
    uint32_t shift = (offset & 2) * 8;
    dword_val &= ~(0xFFFF << shift);
    dword_val |= ((uint32_t)value << shift);
    config_write_dword(bus, slot, func, offset, dword_val);
}

void PCI::config_write_byte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint8_t value) {
    uint32_t dword_val = config_read_dword(bus, slot, func, offset);
    uint32_t shift = (offset & 3) * 8;
    dword_val &= ~(0xFF << shift);
    dword_val |= ((uint32_t)value << shift);
    config_write_dword(bus, slot, func, offset, dword_val);
}

uint32_t PCI::get_bar_size(uint8_t bus, uint8_t slot, uint8_t func, uint8_t bar_index) {
    uint8_t offset = 0x10 + (bar_index * 4);
    uint32_t orig_bar = config_read_dword(bus, slot, func, offset);
    
    config_write_dword(bus, slot, func, offset, 0xFFFFFFFF);
    uint32_t size_mask = config_read_dword(bus, slot, func, offset);
    
    config_write_dword(bus, slot, func, offset, orig_bar);
    
    if (size_mask == 0 || size_mask == 0xFFFFFFFF) return 0;
    
    if ((orig_bar & 0x01) == 0x01) {
        size_mask &= 0xFFFFFFFC;
    } else {
        size_mask &= 0xFFFFFFF0;
    }
    
    return static_cast<uint32_t>(~size_mask) + 1;
}

void PCI::parse_bars(PCIDevice& dev) {
    for (int i = 0; i < 6; i++) {
        uint8_t offset = 0x10 + (i * 4);
        uint32_t bar_val = config_read_dword(dev.bus, dev.slot, dev.func, offset);
        
        if (bar_val == 0) {
            dev.bar[i].size = 0;
            continue;
        }
        
        dev.bar[i].is_param_io = (bar_val & 0x01);
        
        if (dev.bar[i].is_param_io) {
            dev.bar[i].address = bar_val & 0xFFFFFFFC;
            dev.bar[i].is_64_bit = false;
            dev.bar[i].is_prefetchable = false;
        } else {
            uint8_t type = (bar_val >> 1) & 0x03;
            dev.bar[i].address = bar_val & 0xFFFFFFF0;
            dev.bar[i].is_64_bit = (type == 0x02);
            dev.bar[i].is_prefetchable = (bar_val & 0x08);
        }
        
        dev.bar[i].size = get_bar_size(dev.bus, dev.slot, dev.func, i);
    }
}

const char* PCI::get_class_name(uint8_t class_id, uint8_t subclass_id) {
    switch (class_id) {
        case 0x00: return "Unclassified";
        case 0x01: 
            switch (subclass_id) {
                case 0x00: return "SCSI Bus Controller";
                case 0x01: return "IDE Controller";
                case 0x02: return "Floppy Disk Controller";
                case 0x05: return "ATA Controller";
                case 0x06: return "SATA Controller";
                default:   return "Mass Storage Controller";
            }
        case 0x02:
            switch(subclass_id) {
                case 0x00: return "Ethernet Controller";
                default:   return "Network Controller";
            }
        case 0x03:
            switch(subclass_id) {
                case 0x00: return "VGA Compatible Controller";
                default:   return "Display Controller";
            }
        case 0x04: return "Multimedia Controller";
        case 0x05: return "Memory Controller";
        case 0x06:
            switch(subclass_id) {
                case 0x00: return "Host Bridge";
                case 0x01: return "ISA Bridge";
                case 0x04: return "PCI-to-PCI Bridge";
                default:   return "Bridge Device";
            }
        case 0x07: return "Simple Communication Controller";
        case 0x08: return "Base System Peripheral";
        case 0x09: return "Input Device Controller";
        case 0x0C:
            switch(subclass_id) {
                case 0x03: return "USB Controller";
                case 0x05: return "System Management Bus";
                default:   return "Serial Bus Controller";
            }
        default: return "Unknown Device";
    }
}

void PCI::scan_bus() {
    s_device_count = 0;
    
    printf("\n==== PCI ====\n");
    printf("B:S:F | Ven : Dev  | C/S    | IRQ\n");
    printf("------+------------+--------+----\n");
    
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            for (uint8_t func = 0; func < 8; func++) {
                uint16_t vendor_id = config_read_word(bus, slot, func, 0x00);
                if (vendor_id == 0xFFFF) {
                    if (func == 0) {
                        break;
                    }
                    continue;
                }
                
                uint16_t device_id = config_read_word(bus, slot, func, 0x02);
                uint8_t class_id = config_read_byte(bus, slot, func, 0x0B);
                uint8_t subclass_id = config_read_byte(bus, slot, func, 0x0A);
                uint8_t prog_if = config_read_byte(bus, slot, func, 0x09);
                uint8_t irq = config_read_byte(bus, slot, func, 0x3C);
                
                if (s_device_count < 32) {
                    PCIDevice& dev = s_devices[s_device_count++];
                    dev.bus = bus;
                    dev.slot = slot;
                    dev.func = func;
                    dev.vendor_id = vendor_id;
                    dev.device_id = device_id;
                    dev.class_id = class_id;
                    dev.subclass = subclass_id;
                    dev.prog_if = prog_if;
                    dev.irq = irq;
                    
                    parse_bars(dev);
                }
                
                printf("%d:%d:%d | %x : %x | %x : %x | %d\n", 
                    bus, slot, func, vendor_id, device_id, class_id, subclass_id, irq);
                
                if (func == 0) {
                    uint8_t header_type = config_read_byte(bus, slot, func, 0x0E);
                    if ((header_type & 0x80) == 0) {
                        break;
                    }
                }
            }
        }
    }
    printf("----------------------------------\n");
    printf("Total: %d\n", s_device_count);
}

PCIDevice* PCI::get_devices() {
    return s_devices;
}

int PCI::get_device_count() {
    return s_device_count;
}

}
