#include "kernel/e1000.h"
#include "kernel/net.h"
#include "kernel/pci.h"
#include "kernel/pic.h"
#include "kernel/pmm.h"
#include "kernel/vmm.h"
#include "libc.h"

namespace re36 {

namespace {

constexpr uint32_t REG_CTRL  = 0x0000;
constexpr uint32_t REG_STATUS = 0x0008;
constexpr uint32_t REG_ICR   = 0x00C0;
constexpr uint32_t REG_IMS   = 0x00D0;
constexpr uint32_t REG_IMC   = 0x00D8;
constexpr uint32_t REG_RCTL  = 0x0100;
constexpr uint32_t REG_TCTL  = 0x0400;
constexpr uint32_t REG_TIPG  = 0x0410;
constexpr uint32_t REG_RDBAL = 0x2800;
constexpr uint32_t REG_RDBAH = 0x2804;
constexpr uint32_t REG_RDLEN = 0x2808;
constexpr uint32_t REG_RDH   = 0x2810;
constexpr uint32_t REG_RDT   = 0x2818;
constexpr uint32_t REG_TDBAL = 0x3800;
constexpr uint32_t REG_TDBAH = 0x3804;
constexpr uint32_t REG_TDLEN = 0x3808;
constexpr uint32_t REG_TDH   = 0x3810;
constexpr uint32_t REG_TDT   = 0x3818;
constexpr uint32_t REG_RAL   = 0x5400;
constexpr uint32_t REG_RAH   = 0x5404;
constexpr uint32_t REG_MTA   = 0x5200;

constexpr uint32_t CTRL_SLU = 1u << 6;
constexpr uint32_t RCTL_EN = 1u << 1;
constexpr uint32_t RCTL_BAM = 1u << 15;
constexpr uint32_t RCTL_SECRC = 1u << 26;
constexpr uint32_t TCTL_EN = 1u << 1;
constexpr uint32_t TCTL_PSP = 1u << 3;
constexpr uint8_t TX_CMD_EOP = 1u << 0;
constexpr uint8_t TX_CMD_IFCS = 1u << 1;
constexpr uint8_t TX_CMD_RS = 1u << 3;
constexpr uint8_t TX_STATUS_DD = 1u << 0;
constexpr uint8_t RX_STATUS_DD = 1u << 0;
constexpr uint8_t RX_STATUS_EOP = 1u << 1;

constexpr int RX_DESC_COUNT = 32;
constexpr int TX_DESC_COUNT = 8;
constexpr uint32_t RX_BUFFER_SIZE = 2048;

volatile uint32_t* s_mmio = nullptr;
uint32_t s_mmio_base = 0;
uint32_t s_mmio_size = 0;
uint8_t s_irq = 0xFF;
bool s_present = false;
uint8_t s_mac[6];

E1000RxDesc* s_rx_desc = nullptr;
E1000TxDesc* s_tx_desc = nullptr;
uint8_t* s_rx_buffers[RX_DESC_COUNT];
uint8_t* s_tx_buffers[TX_DESC_COUNT];
uint32_t s_rx_index = 0;
uint32_t s_tx_tail = 0;

void zero_page(void* ptr) {
    memset(ptr, 0, 4096);
}

bool valid_mac(const uint8_t* mac) {
    bool all_zero = true;
    bool all_ff = true;
    for (int i = 0; i < 6; i++) {
        if (mac[i] != 0) all_zero = false;
        if (mac[i] != 0xFF) all_ff = false;
    }
    return !all_zero && !all_ff;
}

void pic_clear_mask(uint8_t irq) {
    uint16_t port = irq < 8 ? 0x21 : 0xA1;
    uint8_t line = irq < 8 ? irq : irq - 8;
    uint8_t value = inb(port) & (uint8_t)~(1u << line);
    outb(port, value);
}

} // namespace

uint32_t E1000Driver::read_reg(uint32_t offset) {
    return s_mmio[offset / 4];
}

void E1000Driver::write_reg(uint32_t offset, uint32_t value) {
    s_mmio[offset / 4] = value;
}

void E1000Driver::map_mmio(uint32_t base, uint32_t size) {
    if (size == 0) size = 0x20000;
    uint32_t end = base + size;
    if (end < base) end = 0xFFFFFFFFu;
    for (uint32_t addr = base & ~0xFFFu; addr < end; addr += 4096) {
        VMM::map_page(addr, addr, PAGE_PRESENT | PAGE_WRITABLE | PAGE_CACHEDISABLE);
        if (addr > 0xFFFFFFFFu - 4096) break;
    }
}

PCIDevice* E1000Driver::find_device() {
    PCIDevice* devices = PCI::get_devices();
    int count = PCI::get_device_count();

    for (int i = 0; i < count; i++) {
        if (devices[i].vendor_id == 0x8086 &&
            (devices[i].device_id == 0x100E || devices[i].device_id == 0x100F ||
             devices[i].device_id == 0x1004 || devices[i].device_id == 0x10D3)) {
            return &devices[i];
        }
    }

    for (int i = 0; i < count; i++) {
        if (devices[i].class_id == 0x02 && devices[i].subclass == 0x00) {
            return &devices[i];
        }
    }

    return nullptr;
}

bool E1000Driver::read_mac() {
    uint32_t ral = read_reg(REG_RAL);
    uint32_t rah = read_reg(REG_RAH);

    s_mac[0] = (uint8_t)(ral);
    s_mac[1] = (uint8_t)(ral >> 8);
    s_mac[2] = (uint8_t)(ral >> 16);
    s_mac[3] = (uint8_t)(ral >> 24);
    s_mac[4] = (uint8_t)(rah);
    s_mac[5] = (uint8_t)(rah >> 8);

    if (valid_mac(s_mac)) {
        return true;
    }

    s_mac[0] = 0x52;
    s_mac[1] = 0x54;
    s_mac[2] = 0x00;
    s_mac[3] = 0x12;
    s_mac[4] = 0x34;
    s_mac[5] = 0x56;
    write_reg(REG_RAL, (uint32_t)s_mac[0] | ((uint32_t)s_mac[1] << 8) |
                       ((uint32_t)s_mac[2] << 16) | ((uint32_t)s_mac[3] << 24));
    write_reg(REG_RAH, (uint32_t)s_mac[4] | ((uint32_t)s_mac[5] << 8) | (1u << 31));
    return true;
}

bool E1000Driver::setup_rx() {
    s_rx_desc = (E1000RxDesc*)PhysicalMemoryManager::alloc_frame();
    if (!s_rx_desc) return false;
    zero_page(s_rx_desc);

    for (int i = 0; i < RX_DESC_COUNT; i++) {
        s_rx_buffers[i] = (uint8_t*)PhysicalMemoryManager::alloc_frame();
        if (!s_rx_buffers[i]) return false;
        zero_page(s_rx_buffers[i]);
        s_rx_desc[i].addr = (uint32_t)s_rx_buffers[i];
        s_rx_desc[i].status = 0;
    }

    write_reg(REG_RDBAL, (uint32_t)s_rx_desc);
    write_reg(REG_RDBAH, 0);
    write_reg(REG_RDLEN, RX_DESC_COUNT * sizeof(E1000RxDesc));
    write_reg(REG_RDH, 0);
    write_reg(REG_RDT, RX_DESC_COUNT - 1);
    s_rx_index = 0;

    write_reg(REG_RCTL, RCTL_EN | RCTL_BAM | RCTL_SECRC);
    return true;
}

bool E1000Driver::setup_tx() {
    s_tx_desc = (E1000TxDesc*)PhysicalMemoryManager::alloc_frame();
    if (!s_tx_desc) return false;
    zero_page(s_tx_desc);

    for (int i = 0; i < TX_DESC_COUNT; i++) {
        s_tx_buffers[i] = (uint8_t*)PhysicalMemoryManager::alloc_frame();
        if (!s_tx_buffers[i]) return false;
        zero_page(s_tx_buffers[i]);
        s_tx_desc[i].addr = (uint32_t)s_tx_buffers[i];
        s_tx_desc[i].status = TX_STATUS_DD;
    }

    write_reg(REG_TDBAL, (uint32_t)s_tx_desc);
    write_reg(REG_TDBAH, 0);
    write_reg(REG_TDLEN, TX_DESC_COUNT * sizeof(E1000TxDesc));
    write_reg(REG_TDH, 0);
    write_reg(REG_TDT, 0);
    s_tx_tail = 0;

    write_reg(REG_TIPG, 10 | (8 << 10) | (6 << 20));
    write_reg(REG_TCTL, TCTL_EN | TCTL_PSP | (0x10 << 4) | (0x40 << 12));
    return true;
}

void E1000Driver::init() {
    PCIDevice* dev = find_device();
    if (!dev) {
        printf("[NET] No PCI Ethernet controller found.\n");
        return;
    }

    if (dev->vendor_id != 0x8086) {
        printf("[NET] Ethernet controller %x:%x is not supported by E1000 driver.\n",
               dev->vendor_id, dev->device_id);
        return;
    }

    if (dev->bar[0].is_param_io || dev->bar[0].address == 0) {
        printf("[E1000] BAR0 is not memory mapped.\n");
        return;
    }

    s_mmio_base = dev->bar[0].address;
    s_mmio_size = dev->bar[0].size ? dev->bar[0].size : 0x20000;
    s_irq = dev->irq;

    map_mmio(s_mmio_base, s_mmio_size);
    s_mmio = (volatile uint32_t*)s_mmio_base;

    uint16_t cmd = PCI::config_read_word(dev->bus, dev->slot, dev->func, 0x04);
    PCI::config_write_word(dev->bus, dev->slot, dev->func, 0x04, cmd | 0x0002 | 0x0004);

    write_reg(REG_IMC, 0xFFFFFFFF);
    write_reg(REG_RCTL, 0);
    write_reg(REG_TCTL, 0);

    uint32_t ctrl = read_reg(REG_CTRL);
    write_reg(REG_CTRL, ctrl | CTRL_SLU);

    for (int i = 0; i < 128; i++) {
        write_reg(REG_MTA + (uint32_t)i * 4, 0);
    }

    if (!read_mac()) {
        printf("[E1000] Failed to read MAC address.\n");
        return;
    }

    if (!setup_rx() || !setup_tx()) {
        printf("[E1000] OOM while setting descriptor rings.\n");
        return;
    }

    read_reg(REG_ICR);
    write_reg(REG_IMS, 0x1F6DC);
    if (s_irq < 16) {
        pic_clear_mask(s_irq);
    }

    s_present = true;
    NetStack::attach_device("e1000", s_mac, E1000Driver::send_packet);

    char mac[18];
    NetStack::format_mac(s_mac, mac);
    printf("[E1000] up at %d:%d:%d irq=%d mmio=0x%x mac=%s\n",
           dev->bus, dev->slot, dev->func, s_irq, s_mmio_base, mac);
}

bool E1000Driver::is_present() {
    return s_present;
}

bool E1000Driver::send_packet(const uint8_t* frame, uint16_t length) {
    if (!s_present || !frame || length == 0 || length > NET_MAX_FRAME_SIZE) {
        return false;
    }

    uint32_t tail = s_tx_tail % TX_DESC_COUNT;
    E1000TxDesc* desc = &s_tx_desc[tail];

    int spin = 0;
    while (!(desc->status & TX_STATUS_DD) && spin < 1000000) {
        spin++;
    }
    if (!(desc->status & TX_STATUS_DD)) {
        return false;
    }

    uint16_t tx_len = length < 60 ? 60 : length;
    memcpy(s_tx_buffers[tail], frame, length);
    if (tx_len > length) {
        memset(s_tx_buffers[tail] + length, 0, tx_len - length);
    }

    desc->length = tx_len;
    desc->cmd = TX_CMD_EOP | TX_CMD_IFCS | TX_CMD_RS;
    desc->status = 0;

    s_tx_tail = (tail + 1) % TX_DESC_COUNT;
    write_reg(REG_TDT, s_tx_tail);

    spin = 0;
    while (!(desc->status & TX_STATUS_DD) && spin < 1000000) {
        spin++;
    }

    return (desc->status & TX_STATUS_DD) != 0;
}

void E1000Driver::poll() {
    if (!s_present) return;

    while (s_rx_desc[s_rx_index].status & RX_STATUS_DD) {
        E1000RxDesc* desc = &s_rx_desc[s_rx_index];
        if ((desc->status & RX_STATUS_EOP) && desc->errors == 0 &&
            desc->length >= 14 && desc->length <= RX_BUFFER_SIZE) {
            NetStack::receive_frame(s_rx_buffers[s_rx_index], desc->length);
        }

        desc->status = 0;
        desc->errors = 0;
        write_reg(REG_RDT, s_rx_index);
        s_rx_index = (s_rx_index + 1) % RX_DESC_COUNT;
    }
}

void E1000Driver::handle_interrupt(uint8_t irq) {
    if (!s_present || irq != s_irq) return;

    uint32_t icr = read_reg(REG_ICR);
    if (icr == 0) return;

    if (icr & (1u << 2)) {
        uint32_t status = read_reg(REG_STATUS);
        (void)status;
    }

    poll();
}

} // namespace re36
