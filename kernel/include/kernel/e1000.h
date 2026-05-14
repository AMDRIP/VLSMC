#pragma once

#include <stdint.h>
#include "kernel/pci.h"

namespace re36 {

struct E1000RxDesc {
    uint64_t addr;
    uint16_t length;
    uint16_t checksum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
} __attribute__((packed));

struct E1000TxDesc {
    uint64_t addr;
    uint16_t length;
    uint8_t cso;
    uint8_t cmd;
    uint8_t status;
    uint8_t css;
    uint16_t special;
} __attribute__((packed));

class E1000Driver {
public:
    static void init();
    static bool is_present();
    static bool send_packet(const uint8_t* frame, uint16_t length);
    static void handle_interrupt(uint8_t irq);
    static void poll();

private:
    static PCIDevice* find_device();
    static bool read_mac();
    static bool setup_rx();
    static bool setup_tx();
    static uint32_t read_reg(uint32_t offset);
    static void write_reg(uint32_t offset, uint32_t value);
    static void map_mmio(uint32_t base, uint32_t size);
};

} // namespace re36
