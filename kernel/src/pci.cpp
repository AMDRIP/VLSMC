#include "kernel/pci.h"
#include "kernel/pic.h" // For outportd / inportd equivalent, wait, let's use inw/outw from a util header if available; PIC might not have 32-bit versions.
#include "libc.h"

namespace re36 {

// Inlines for 32-bit I/O ports
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

void PCI::init() {
    // No specific initialization required for direct port I/O
}

uint32_t PCI::config_read_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
 
    // Create configuration address as per PCI spec
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
 
    // Write out the address
    outportl(PCI_CONFIG_ADDRESS, address);
 
    // Read in the data
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
    printf("\n==== PCI Devices ====\n");
    printf("Bus : Slot : Func | VendorID : DeviceID | Class\n");
    printf("----+------+------+----------+----------+----------------------------\n");
    
    int count = 0;
    
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            for (uint8_t func = 0; func < 8; func++) {
                uint16_t vendor_id = config_read_word(bus, slot, func, 0x00);
                if (vendor_id == 0xFFFF) {
                    if (func == 0) {
                        break; // No device in this slot
                    }
                    continue; // Function doesn't exist
                }
                
                uint16_t device_id = config_read_word(bus, slot, func, 0x02);
                uint8_t class_id = config_read_byte(bus, slot, func, 0x0B);
                uint8_t subclass_id = config_read_byte(bus, slot, func, 0x0A);
                
                const char* desc = get_class_name(class_id, subclass_id);
                
                // Formatted output
                printf(" %d  :  %d  :  %d   |  %x    :  %x    | %s\n", 
                    bus, slot, func, vendor_id, device_id, desc);
                
                count++;
                
                // Check if device is multi-function. If not, don't query other funcs.
                if (func == 0) {
                    uint8_t header_type = config_read_byte(bus, slot, func, 0x0E);
                    if ((header_type & 0x80) == 0) {
                        break; // Not a multi-function device
                    }
                }
            }
        }
    }
    printf("---------------------------------------------------------------------\n");
    printf("Total PCI Devices Found: %d\n", count);
}

} // namespace re36
