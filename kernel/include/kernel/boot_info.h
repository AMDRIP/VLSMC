#pragma once

#include <stdint.h>

struct BootInfo {
    uint8_t  boot_drive;
    uint8_t  video_mode;
    uint16_t mem_below_16m_kb;
    uint16_t mem_above_16m_64kb;
    uint32_t magic;
} __attribute__((packed));

#define BOOT_INFO_ADDR   0x0500
#define BOOT_INFO_MAGIC  0xB0071AF0

static inline BootInfo* get_boot_info() {
    return (BootInfo*)BOOT_INFO_ADDR;
}
