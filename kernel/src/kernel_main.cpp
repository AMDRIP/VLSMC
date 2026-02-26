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
    dbg[5] = 0x4F36; // '6' — VMM

    re36::VMM::init();
    dbg[6] = 0x4F37; // '7' — Scheduler

    re36::TaskScheduler::init();
    dbg[7] = 0x4F38; // '8' — Timer
    dbg[8] = 0x4F39; // '9' — Timer

    re36::Timer::init(100);
    dbg[9] = 0x4F41; // 'A' — TSS

    re36::TSS::init(0x90000);
    dbg[10] = 0x4F42; // 'B' — Syscall

    re36::syscall_gate_init();
    dbg[11] = 0x4F43; // 'C' — ATA/FS

    if (re36::ATA::init()) {
        re36::Fat16::init();
    }

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
    printf("-> PIT Timer Initialized (100 Hz)\n");
    printf("-> Task Scheduler Initialized (Priority RR)\n");
    printf("-> Event Channel System Ready\n");
    printf("-> VMM Paging Enabled (Kernel Supervisor-only)\n");
    printf("-> TSS Loaded (Ring 3 Ready)\n");
    printf("-> Syscall Gate (int 0x80) Registered\n");
    if (re36::ATA::is_present()) {
        printf("-> ATA Primary Master Detected\n");
        if (re36::Fat16::is_mounted())
            printf("-> FAT16 Filesystem Mounted\n");
    } else {
        set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        printf("-> ATA: No IDE disk detected\n");
        set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    }
    printf("-> Interrupts Enabled (STI)\n\n");
    
    set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    re36::thread_create("idle", idle_thread, 255);
    re36::elf_exec("STACKBM.ELF");
    re36::thread_create("shell", shell_thread, 1);

    set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    printf("Spawned threads: idle (pri=255), shell (pri=1)\n");
    printf("Switching to shell thread...\n\n");
    set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    while (true) {
        asm volatile("hlt");
    }
}
