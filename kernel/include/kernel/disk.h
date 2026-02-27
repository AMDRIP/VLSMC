#pragma once

#include <stdint.h>
#include <stddef.h>

namespace re36 {

class Disk {
public:
    static bool is_present();
    static bool read_sectors(uint64_t lba, uint32_t count, void* buffer);
    static bool write_sectors(uint64_t lba, uint32_t count, const void* buffer);
};

} // namespace re36
