#pragma once

#include <stdint.h>

namespace re36 {

struct PCIBAR {
    bool is_param_io;
    bool is_64_bit;
    bool is_prefetchable;
    uint32_t address;
    uint32_t size;
};

struct PCIDevice {
    uint8_t bus;
    uint8_t slot;
    uint8_t func;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_id;
    uint8_t subclass;
    uint8_t prog_if;
    uint8_t irq;
    
    PCIBAR bar[6];
};

class PCI {
public:
    static void init();
    
    static uint32_t config_read_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
    static uint16_t config_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
    static uint8_t config_read_byte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

    static void config_write_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value);
    static void config_write_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t value);
    static void config_write_byte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint8_t value);

    static void scan_bus();
    
    static const char* get_class_name(uint8_t class_id, uint8_t subclass_id);
    
    static PCIDevice* get_devices();
    static int get_device_count();

private:
    static void parse_bars(PCIDevice& dev);
    static uint32_t get_bar_size(uint8_t bus, uint8_t slot, uint8_t func, uint8_t bar_index);
};

} 
