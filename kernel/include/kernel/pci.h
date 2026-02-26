#pragma once

#include <stdint.h>

namespace re36 {

struct PCIDevice {
    uint8_t bus;
    uint8_t slot;
    uint8_t func;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_id;
    uint8_t subclass;
};

class PCI {
public:
    static void init(); // Initialization if necessary
    
    // Reads a 32-bit value from PCI configuration space
    static uint32_t config_read_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
    
    // Reads a 16-bit value from PCI configuration space
    static uint16_t config_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
    
    // Reads an 8-bit value from PCI configuration space
    static uint8_t config_read_byte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

    // Scans all PCI buses and prints connected devices
    static void scan_bus();
    
    // Returns a human-readable string for the device Class/Subclass
    static const char* get_class_name(uint8_t class_id, uint8_t subclass_id);
};

} // namespace re36
