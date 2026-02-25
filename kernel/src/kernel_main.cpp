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
    printf("Advanced Keyboard Driver Demo (Ring 0)\n");
    printf("- Type text to see it appear.\n");
    printf("- Press Alt+Shift to switch Layouts (QWERTY <-> Dvorak).\n");
    printf("- Try Ctrl+C or F1-F10 keys.\n");
    printf("- Type 'ps' to list threads, 'clear' to clear screen.\n\n");
    printf("> ");

    re36::string input_buffer;

    while (true) {
        char c = getchar();
        
        if (c == '\n') {
            if (input_buffer == "hello") {
                printf("Hello to you too, Kernel Hacker!\n> ");
            } else if (input_buffer == "clear") {
                for (int i = 0; i < 80 * 25; i++) {
                    vga_buffer[i] = (uint16_t(' ') | (0x1F << 8)); 
                }
                printf("\n> ");
            } else if (input_buffer == "ps") {
                re36::TaskScheduler::print_threads();
                printf("> ");
            } else if (input_buffer == "ticks") {
                printf("Timer ticks: %u\n> ", re36::Timer::get_ticks());
            } else if (input_buffer == "meminfo") {
                printf("Free RAM: %u KB\n", re36::PhysicalMemoryManager::get_free_memory() / 1024);
                printf("Used RAM: %u KB\n", re36::PhysicalMemoryManager::get_used_memory() / 1024);
                printf("Paging: Enabled (CR3 = 0x%x)\n> ", (uint32_t)re36::VMM::get_current_directory());
            } else if (input_buffer == "syscall") {
                printf("Testing int 0x80 (SYS_GETPID)...\n");
                uint32_t tid;
                asm volatile("mov $5, %%eax; int $0x80; mov %%eax, %0" : "=r"(tid) :: "eax");
                printf("Syscall returned TID = %d\n> ", tid);
            } else if (input_buffer == "help") {
                printf("Commands: ps, ticks, meminfo, syscall, ring3, ls, cat, clear, hello, help\n> ");
            } else if (input_buffer == "ring3") {
                printf("Launching Ring 3 user process...\n");
                re36::thread_create("user0", user_thread_entry, 10);
                printf("> ");
            } else if (input_buffer == "ls") {
                re36::Fat16::list_root();
                printf("> ");
            } else if (input_buffer.size() > 4 && input_buffer.c_str()[0] == 'c' && input_buffer.c_str()[1] == 'a' && input_buffer.c_str()[2] == 't' && input_buffer.c_str()[3] == ' ') {
                const char* fname = input_buffer.c_str() + 4;
                static uint8_t file_buf[4096];
                int bytes = re36::Fat16::read_file(fname, file_buf, sizeof(file_buf) - 1);
                if (bytes < 0) {
                    printf("File not found: %s\n> ", fname);
                } else {
                    file_buf[bytes] = '\0';
                    printf("%s\n> ", (const char*)file_buf);
                }
            } else if (input_buffer.size() > 0) {
                printf("Unknown command: %s\n> ", input_buffer.c_str());
            } else {
                printf("> ");
            }
            input_buffer = "";
        } else if (c == '\b') {
            if (input_buffer.size() > 0) {
                input_buffer[input_buffer.size() - 1] = '\0';
            }
        } else if (c >= 32 && c <= 126) {
            char temp[2] = {c, '\0'};
            input_buffer += temp;
        }
    }
}

extern "C" void kernel_main() {
    volatile uint16_t* dbg = (volatile uint16_t*)0xB8000;
    dbg[0] = 0x4F31; // '1' красный на белом — IDT

    re36::init_idt();
    dbg[1] = 0x4F32; // '2' — PIC

    re36::pic_remap(0x20, 0x28);
    dbg[2] = 0x4F33; // '3' — PMM

    re36::PhysicalMemoryManager::init(0x20000, 32 * 1024 * 1024);
    re36::PhysicalMemoryManager::set_region_free(0x100000, 31 * 1024 * 1024);
    dbg[3] = 0x4F34; // '4' — kmalloc

    re36::kmalloc_init();
    dbg[4] = 0x4F35; // '5' — Keyboard

    re36::KeyboardDriver::init();
    dbg[5] = 0x4F36; // '6' — Scheduler

    re36::TaskScheduler::init();
    dbg[6] = 0x4F37; // '7' — Timer

    re36::Timer::init(100);
    dbg[7] = 0x4F38; // '8' — Events

    re36::EventSystem::init();
    dbg[8] = 0x4F39; // '9' — VMM

    re36::VMM::init();
    dbg[9] = 0x4F41; // 'A' — TSS

    re36::TSS::init(0x90000);
    dbg[10] = 0x4F42; // 'B' — Syscall

    re36::syscall_gate_init();
    dbg[11] = 0x4F43; // 'C' — ATA/FS

    if (re36::ATA::init()) {
        re36::Fat16::init();
    }
    dbg[12] = 0x4F44; // 'D' — STI

    asm volatile("sti");
    dbg[13] = 0x4F4F; // 'O' — OK!
    for (int i = 0; i < 80 * 25; i++) {
        vga_buffer[i] = (uint16_t(' ') | (0x1F << 8));
    }

    printf("==========================================\n");
    printf("   RAND Elecorner 36 OS (Bare-Metal)      \n");
    printf("==========================================\n\n");

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
        printf("-> ATA: No IDE disk detected\n");
    }
    printf("-> Interrupts Enabled (STI)\n\n");

    re36::thread_create("idle", idle_thread, 255);
    re36::thread_create("shell", shell_thread, 1);

    printf("Spawned threads: idle (pri=255), shell (pri=1)\n");
    printf("Switching to shell thread...\n\n");

    while (true) {
        asm volatile("hlt");
    }
}
