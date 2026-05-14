#include <stdint.h>
#include <stddef.h>
#include "kernel/idt.h"
#include "kernel/pmm.h"
#include "kernel/pic.h"
#include "kernel/kmalloc.h"
#include "kernel/vector.h"
#include "kernel/string.h"
#include "kernel/keyboard.h"
#include "kernel/timer.h"
#include "kernel/task_scheduler.h"
#include "kernel/event_channel.h"
#include "kernel/vmm.h"
#include "kernel/tss.h"
#include "kernel/syscall_gate.h"
#include "kernel/usermode.h"
#include "kernel/ata.h"
#include "kernel/fat16.h"
#include "kernel/elf_loader.h"
#include "kernel/rtc.h"
#include "kernel/shell.h"
#include "kernel/selftest.h"
#include "kernel/pci.h"
#include "kernel/ahci.h"
#include "kernel/disk.h"
#include "kernel/memory_validator.h"
#include "kernel/mouse.h"
#include "kernel/bga.h"
#include "kernel/boot_info.h"
#include "kernel/vfs.h"
#include "kernel/page_cache.h"
#include "kernel/net.h"
#include "kernel/e1000.h"
#include "libc.h"

static volatile uint16_t* vga_buffer = (volatile uint16_t*)0xB8000;

namespace {

constexpr uint32_t kLowMemoryCeiling = 0x100000;
constexpr uint32_t kDirectMapLimit = KERNEL_SPACE_END;
constexpr uint32_t kMaxManagedMemoryLimit = 0xFFFFF000;

uint32_t align_up(uint32_t value, uint32_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

bool clip_memory_entry(const BootMemoryMapEntry& entry, uint32_t window_end,
                       uint32_t* clipped_start, uint32_t* clipped_end) {
    if (entry.length_low == 0 && entry.length_high == 0) {
        return false;
    }

    if (entry.base_high != 0) {
        return false;
    }

    uint64_t start = entry.base_low;
    uint64_t end = start + ((uint64_t)entry.length_high << 32) + entry.length_low;
    if (start >= window_end) {
        return false;
    }

    if (end > window_end) {
        end = window_end;
    }

    if (end <= start) {
        return false;
    }

    *clipped_start = (uint32_t)start;
    *clipped_end = (uint32_t)end;
    return true;
}

uint32_t compute_managed_memory_limit(const BootInfo* boot_info) {
    uint32_t highest_usable_end = 0;

    for (uint16_t index = 0; index < boot_info->memory_map_entry_count; ++index) {
        const BootMemoryMapEntry& entry = boot_info->memory_map[index];
        if (entry.type != BOOT_MEMORY_TYPE_USABLE) {
            continue;
        }

        uint32_t start = 0;
        uint32_t end = 0;
        if (!clip_memory_entry(entry, kMaxManagedMemoryLimit, &start, &end)) {
            continue;
        }

        if (end > highest_usable_end) {
            highest_usable_end = end;
        }
    }

    return highest_usable_end;
}

uint32_t compute_total_usable_memory(const BootInfo* boot_info) {
    uint64_t total_usable = 0;

    for (uint16_t index = 0; index < boot_info->memory_map_entry_count; ++index) {
        const BootMemoryMapEntry& entry = boot_info->memory_map[index];
        if (entry.type != BOOT_MEMORY_TYPE_USABLE) {
            continue;
        }

        uint32_t start = 0;
        uint32_t end = 0;
        if (!clip_memory_entry(entry, kMaxManagedMemoryLimit, &start, &end)) {
            continue;
        }

        total_usable += end - start;
    }

    if (total_usable > 0xFFFFFFFFULL) {
        return 0xFFFFFFFF;
    }
    return (uint32_t)total_usable;
}

uint32_t find_pmm_metadata_base(const BootInfo* boot_info, uint32_t metadata_size, uint32_t metadata_ceiling) {
    uint32_t min_base = align_up((uint32_t)&_kernel_end, PMM_FRAME_SIZE);
    if (min_base < kLowMemoryCeiling) {
        min_base = kLowMemoryCeiling;
    }

    for (uint16_t index = 0; index < boot_info->memory_map_entry_count; ++index) {
        const BootMemoryMapEntry& entry = boot_info->memory_map[index];
        if (entry.type != BOOT_MEMORY_TYPE_USABLE) {
            continue;
        }

        uint32_t start = 0;
        uint32_t end = 0;
        if (!clip_memory_entry(entry, metadata_ceiling, &start, &end)) {
            continue;
        }

        if (end <= min_base) {
            continue;
        }

        if (start < min_base) {
            start = min_base;
        }
        start = align_up(start, PMM_FRAME_SIZE);

        if (end > start && end - start >= metadata_size) {
            return start;
        }
    }

    return 0;
}

} // namespace

void idle_thread() {
    while (true) {
        asm volatile("hlt");
    }
}

void user_thread_entry() {
    re36::enter_usermode();
}

void shell_thread() {
    re36::shell_main();
}

extern "C" void kernel_main(BootInfo* boot_info) {
    serial_init();
    volatile uint16_t* dbg = (volatile uint16_t*)0xB8000;
    dbg[0] = 0x4F31;

    if (!boot_info_is_valid(boot_info)) {
        printf("FATAL: Invalid BootInfo contract!\n");
        while (true) asm volatile("cli; hlt");
    }

    re36::init_idt();
    dbg[1] = 0x4F32;

    re36::pic_remap(0x20, 0x28);
    dbg[2] = 0x4F33;

    uint32_t managed_memory_limit = compute_managed_memory_limit(boot_info);
    if (managed_memory_limit <= kLowMemoryCeiling) {
        printf("FATAL: BootInfo has no usable RAM above 1 MiB!\n");
        while (true) asm volatile("cli; hlt");
    }

    uint32_t pmm_metadata_size = re36::PhysicalMemoryManager::calculate_metadata_size(managed_memory_limit);
    uint32_t metadata_ceiling = managed_memory_limit < kDirectMapLimit ? managed_memory_limit : kDirectMapLimit;
    uint32_t pmm_bitmap_addr = find_pmm_metadata_base(boot_info, pmm_metadata_size, metadata_ceiling);
    if (pmm_bitmap_addr == 0) {
        printf("FATAL: Cannot place PMM metadata in usable RAM!\n");
        while (true) asm volatile("cli; hlt");
    }

    re36::PhysicalMemoryManager::init(pmm_bitmap_addr, managed_memory_limit, kDirectMapLimit);

    for (uint16_t index = 0; index < boot_info->memory_map_entry_count; ++index) {
        const BootMemoryMapEntry& entry = boot_info->memory_map[index];
        if (entry.type != BOOT_MEMORY_TYPE_USABLE) {
            continue;
        }

        uint32_t start = 0;
        uint32_t end = 0;
        if (!clip_memory_entry(entry, managed_memory_limit, &start, &end)) {
            continue;
        }

        if (end <= kLowMemoryCeiling) {
            continue;
        }

        if (start < kLowMemoryCeiling) {
            start = kLowMemoryCeiling;
        }

        if (end > start) {
            re36::PhysicalMemoryManager::set_region_free(start, end - start);
        }
    }

    uint32_t pmm_end_addr = align_up(pmm_bitmap_addr + pmm_metadata_size, PMM_FRAME_SIZE);
    re36::PhysicalMemoryManager::set_region_used((uint32_t)&_kernel_start, pmm_end_addr - (uint32_t)&_kernel_start);
    if (boot_info->boot_stack_top >= boot_info->boot_stack_size) {
        re36::PhysicalMemoryManager::set_region_used(boot_info->boot_stack_top - boot_info->boot_stack_size,
                                                     boot_info->boot_stack_size);
    }
    dbg[3] = 0x4F34;

    re36::kmalloc_init();
    re36::EventSystem::init();
    dbg[4] = 0x4F35;

    re36::KeyboardDriver::init();
    re36::MouseDriver::init();
    dbg[5] = 0x4F36;

    re36::VMM::init();
    re36::PageCache::init();
    dbg[6] = 0x4F37;

    if (!re36::MemoryValidator::run_all_tests()) {
        printf("FATAL: Memory subsystem validation failed!\n");
        while (true) asm volatile("cli; hlt");
    }

    re36::TaskScheduler::init();
    dbg[7] = 0x4F38;
    dbg[8] = 0x4F39;

    re36::Timer::init(100);
    dbg[9] = 0x4F41;

    re36::TSS::init(0x90000);
    dbg[10] = 0x4F42;

    re36::syscall_gate_init();
    dbg[11] = 0x4F43;

    re36::RTC::init(false);
    dbg[12] = 0x4F44;

    asm volatile("sti");
    dbg[13] = 0x4F4F;
    for (int i = 0; i < 80 * 25; i++) {
        vga_buffer[i] = (uint16_t(' ') | (0x1F << 8));
    }

    set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    printf("==========================================\n");
    printf("   RAND Elecorner 36 OS (Bare-Metal)      \n");
    printf("==========================================\n\n");

    set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    uint32_t managed_memory_mb = re36::PhysicalMemoryManager::get_managed_memory_limit() / (1024 * 1024);
    uint32_t direct_memory_mb = re36::PhysicalMemoryManager::get_direct_mapped_memory() / (1024 * 1024);
    uint32_t high_memory_mb = re36::PhysicalMemoryManager::get_high_memory() / (1024 * 1024);
    uint32_t usable_memory_mb = compute_total_usable_memory(boot_info) / (1024 * 1024);
    if (usable_memory_mb > managed_memory_mb) {
        printf("-> PMM Initialized (%u MB managed of %u MB usable)\n", managed_memory_mb, usable_memory_mb);
    } else {
        printf("-> PMM Initialized (%u MB usable RAM)\n", managed_memory_mb);
    }
    printf("-> Memory pools: %u MB direct, %u MB high\n", direct_memory_mb, high_memory_mb);
    printf("-> Heap Initialized\n");
    printf("-> Keyboard Driver (Ring 0) Loaded via IRQ1\n");
    printf("-> PS/2 Mouse Driver (Ring 0) Loaded via IRQ12\n");
    printf("-> PIT Timer Initialized (100 Hz)\n");
    printf("-> Task Scheduler Initialized (Priority RR)\n");
    printf("-> Event Channel System Ready\n");
    printf("-> ATA Disk Controller Ready\n");
    printf("-> Network Stack Ready\n");

    re36::ATA::init();
    re36::PCI::scan_bus();
    re36::NetStack::init();
    re36::E1000Driver::init();
    re36::AHCIDriver::init();

    re36::BgaDriver::init(1024, 768, 32);

    re36::vfs_init();
    re36::vfs_register(&re36::fat16_driver);

    if (re36::Disk::is_present()) {
        if (re36::vfs_mount("fat16", "/", nullptr) == 0) {
            printf("-> FAT16 Filesystem Mounted via VFS on /\n");
        } else {
            printf("-> VFS FAT16 Mount Failed!\n");
        }
    } else {
        printf("-> No block device for VFS mount\n");
    }

    printf("======================================\n");
    if (re36::Disk::is_present()) {
        printf("-> Primary Disk Detected (%s)\n", re36::AHCIDriver::is_present() ? "AHCI" : "ATA IDE");
    } else {
        set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        printf("-> No disk detected (ATA nor AHCI)\n");
        set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    }
    printf("-> Interrupts Enabled (STI)\n\n");

    set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    re36::thread_create("idle", idle_thread, 255);
    int shell_tid = re36::thread_create("shell", shell_thread, 1);
    if (shell_tid >= 0) {
        re36::threads[shell_tid].is_driver = true;
    }

    set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    printf("Spawned threads: idle (pri=255), shell (pri=1)\n");
    printf("Switching to shell thread...\n\n");
    set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    while (true) {
        asm volatile("sti; hlt");
    }
}
