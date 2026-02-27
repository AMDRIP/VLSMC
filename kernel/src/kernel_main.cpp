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
#include "kernel/ata.h"
#include "kernel/fat16.h"
#include "kernel/vfs.h"
#include "libc.h"

static volatile uint16_t* vga_buffer = (volatile uint16_t*)0xB8000;

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

extern "C" void kernel_main() {
    serial_init();
    volatile uint16_t* dbg = (volatile uint16_t*)0xB8000;
    dbg[0] = 0x4F31;

    re36::init_idt();
    dbg[1] = 0x4F32; // '2' — PIC

    re36::pic_remap(0x20, 0x28);
    dbg[2] = 0x4F33; // '3' — PMM

    uint32_t pmm_bitmap_addr = ((uint32_t)&_kernel_end + 0xFFF) & ~0xFFF;
    re36::PhysicalMemoryManager::init(pmm_bitmap_addr, 32 * 1024 * 1024);
    re36::PhysicalMemoryManager::set_region_free(0x100000, 31 * 1024 * 1024);
    dbg[3] = 0x4F34; // '4' — kmalloc

    re36::EventSystem::init();
    dbg[4] = 0x4F35; // '5' — Keyboard

    re36::KeyboardDriver::init();
    re36::MouseDriver::init();
    dbg[5] = 0x4F36; // '6' — VMM

    re36::VMM::init();
    dbg[6] = 0x4F37; // '7' — Scheduler

    if (!re36::MemoryValidator::run_all_tests()) {
        printf("FATAL: Memory subsystem validation failed!\n");
        while(1) asm volatile("cli; hlt");
    }

    re36::TaskScheduler::init();
    dbg[7] = 0x4F38; // '8' — Timer
    dbg[8] = 0x4F39; // '9' — Timer

    re36::Timer::init(100);
    dbg[9] = 0x4F41; // 'A' — TSS

    re36::TSS::init(0x90000);
    dbg[10] = 0x4F42; // 'B' — Syscall

    re36::syscall_gate_init();
    dbg[11] = 0x4F43; // 'C' — ATA/FS

    // Disk initialization happens later now

    re36::RTC::init(false);
    dbg[12] = 0x4F44; // 'D' — STI

    asm volatile("sti");
    dbg[13] = 0x4F4F; // 'O' — OK!
    for (int i = 0; i < 80 * 25; i++) {
        vga_buffer[i] = (uint16_t(' ') | (0x1F << 8));
    }

    set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    printf("==========================================\n");
    printf("   RAND Elecorner 36 OS (Bare-Metal)      \n");
    printf("==========================================\n\n");

    set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    printf("-> PMM Initialized (32 MB RAM)\n");
    printf("-> Heap Initialized\n");
    printf("-> Keyboard Driver (Ring 0) Loaded via IRQ1\n");
    printf("-> PS/2 Mouse Driver (Ring 0) Loaded via IRQ12\n");
    printf("-> PIT Timer Initialized (100 Hz)\n");
    printf("-> Task Scheduler Initialized (Priority RR)\n");
    printf("-> Event Channel System Ready\n");
    printf("-> ATA Disk Controller Ready\n");
    
    re36::ATA::init();
    re36::PCI::scan_bus();
    re36::AHCIDriver::init();
    
    // Initialize Video Mode (1024x768x32)
    re36::BgaDriver::init(1024, 768, 32);

    // Initialize Virtual File System
    re36::vfs_init();
    re36::vfs_register(&re36::fat16_driver);

    // Mount the disk via VFS driver interface
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
    re36::thread_create("shell", shell_thread, 1);

    set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    printf("Spawned threads: idle (pri=255), shell (pri=1)\n");
    printf("Switching to shell thread...\n\n");
    set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    while (true) {
        asm volatile("hlt");
    }
}
