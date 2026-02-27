#include "kernel/disk.h"
#include "kernel/ahci.h"
#include "kernel/ata.h"
#include "kernel/vmm.h"
#include "libc.h"

namespace re36 {

bool Disk::is_present() {
    return AHCIDriver::is_present() || ATA::is_present();
}

bool Disk::read_sectors(uint64_t lba, uint32_t count, void* buffer) {
    if (AHCIDriver::is_present()) {
        uint32_t phys = VMM::get_physical((uint32_t)buffer);
        return AHCIDriver::read((uint8_t)AHCIDriver::get_primary_port(), lba, count, (void*)phys);
    } else if (ATA::is_present()) {
        return ATA::read_sectors((uint32_t)lba, count, buffer);
    }
    return false;
}

bool Disk::write_sectors(uint64_t lba, uint32_t count, const void* buffer) {
    if (AHCIDriver::is_present()) {
        uint32_t phys = VMM::get_physical((uint32_t)buffer);
        // We cast const void* to void*, because AHCIDriver::write just needs the PRDT pointer
        return AHCIDriver::write((uint8_t)AHCIDriver::get_primary_port(), lba, count, (void*)phys);
    } else if (ATA::is_present()) {
        return ATA::write_sectors((uint32_t)lba, count, buffer);
    }
    return false;
}

} // namespace re36
