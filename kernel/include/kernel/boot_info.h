#pragma once

#include <stdint.h>
#include <stddef.h>

constexpr uint32_t BOOT_INFO_ADDR = 0x0500;
constexpr uint32_t BOOT_INFO_MAGIC = 0xB0071AF0;
constexpr uint16_t BOOT_INFO_VERSION = 1;
constexpr uint16_t BOOT_INFO_MAX_MEMORY_MAP_ENTRIES = 32;

constexpr uint32_t BOOT_INFO_FLAG_E820_VALID = 1u << 0;
constexpr uint32_t BOOT_INFO_FLAG_MEMORY_FALLBACK = 1u << 1;
constexpr uint32_t BOOT_INFO_FLAG_MEMORY_MAP_TRUNCATED = 1u << 2;

constexpr uint32_t BOOT_MEMORY_TYPE_USABLE = 1;
constexpr uint32_t BOOT_MEMORY_TYPE_RESERVED = 2;
constexpr uint32_t BOOT_MEMORY_TYPE_ACPI_RECLAIM = 3;
constexpr uint32_t BOOT_MEMORY_TYPE_ACPI_NVS = 4;
constexpr uint32_t BOOT_MEMORY_TYPE_BAD = 5;

struct BootMemoryMapEntry {
    uint32_t base_low;
    uint32_t base_high;
    uint32_t length_low;
    uint32_t length_high;
    uint32_t type;
} __attribute__((packed));

struct BootInfo {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    uint32_t flags;
    uint8_t boot_drive;
    uint8_t video_mode;
    uint16_t memory_map_entry_count;
    uint16_t memory_map_entry_size;
    uint16_t reserved0;
    uint32_t conventional_memory_kb;
    uint32_t kernel_load_addr;
    uint32_t kernel_load_size;
    uint32_t kernel_entry_addr;
    uint32_t boot_stack_top;
    uint32_t boot_stack_size;
    uint32_t reserved1;
    BootMemoryMapEntry memory_map[BOOT_INFO_MAX_MEMORY_MAP_ENTRIES];
} __attribute__((packed));

static_assert(sizeof(BootMemoryMapEntry) == 20, "Unexpected BootMemoryMapEntry size");
static_assert(offsetof(BootInfo, memory_map) == 48, "Unexpected BootInfo header size");

static inline BootInfo* get_boot_info() {
    return reinterpret_cast<BootInfo*>(BOOT_INFO_ADDR);
}

static inline const BootInfo* get_boot_info_const() {
    return reinterpret_cast<const BootInfo*>(BOOT_INFO_ADDR);
}

static inline bool boot_info_is_valid(const BootInfo* boot_info) {
    return boot_info &&
           boot_info->magic == BOOT_INFO_MAGIC &&
           boot_info->version == BOOT_INFO_VERSION &&
           boot_info->size >= sizeof(BootInfo) &&
           boot_info->memory_map_entry_size == sizeof(BootMemoryMapEntry) &&
           boot_info->memory_map_entry_count <= BOOT_INFO_MAX_MEMORY_MAP_ENTRIES;
}
