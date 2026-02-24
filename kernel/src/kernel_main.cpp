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
#include "libc.h"

static volatile uint16_t* vga_buffer = (volatile uint16_t*)0xB8000;

void idle_thread() {
    while (true) {
        asm volatile("hlt");
    }
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
    re36::init_idt();

    re36::pic_remap(0x20, 0x28);

    re36::PhysicalMemoryManager::init(0x20000, 32 * 1024 * 1024);
    re36::PhysicalMemoryManager::set_region_free(0x100000, 31 * 1024 * 1024);

    re36::kmalloc_init();

    re36::KeyboardDriver::init();

    re36::TaskScheduler::init();

    re36::Timer::init(100);

    re36::EventSystem::init();

    asm volatile("sti");

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
    printf("-> Interrupts Enabled (STI)\n\n");

    re36::thread_create("idle", idle_thread, 255);
    re36::thread_create("shell", shell_thread, 1);

    printf("Spawned threads: idle (pri=255), shell (pri=1)\n");
    printf("Switching to shell thread...\n\n");

    while (true) {
        asm volatile("hlt");
    }
}
