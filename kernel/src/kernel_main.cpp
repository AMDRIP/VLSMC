#include <stdint.h>
#include <stddef.h>
#include "kernel/idt.h"
#include "kernel/pmm.h"
#include "kernel/pic.h"
#include "kernel/kmalloc.h"
#include "kernel/vector.h"
#include "kernel/string.h"
#include "libc.h" // Для printf

extern "C" void kernel_main() {
    // 1. Инициализируем прерывания (IDT)
    re36::init_idt();

    // 2. Перемапливаем PIC
    re36::pic_remap(0x20, 0x28);

    // 3. Включаем аппаратные прерывания
    asm volatile("sti");

    // 4. Инициализация Physical Memory Manager
    re36::PhysicalMemoryManager::init(0x20000, 32 * 1024 * 1024);
    re36::PhysicalMemoryManager::set_region_free(0x100000, 31 * 1024 * 1024);

    // 5. Инициализация Кучи (Heap) 
    re36::kmalloc_init();

    // Очистим экран синим фоном напрямую разок (чтобы было красиво, как BIOS)
    volatile uint16_t* vga_buffer = (volatile uint16_t*)0xB8000;
    for (int i = 0; i < 80 * 25; i++) {
        vga_buffer[i] = (uint16_t(' ') | (0x1F << 8)); // Синий фон
    }

    printf("==========================================\n");
    printf("   RAND Elecorner 36 OS (Bare-Metal)      \n");
    printf("==========================================\n\n");

    printf("-> PMM Initialized (32 MB RAM)\n");
    printf("-> Heap Initialized\n");
    printf("-> Interrupts Enabled (STI)\n\n");

    // 6. Тест re36::vector
    printf("[Testing re36::vector]\n");
    re36::vector<int> my_vector;
    my_vector.push_back(42);
    my_vector.push_back(1337);
    my_vector.push_back(2026);
    
    for (size_t i = 0; i < my_vector.size(); i++) {
        printf("   my_vector[%d] = %d (Hex: 0x%x)\n", i, my_vector[i], my_vector[i]);
    }

    // 7. Тест re36::string
    printf("\n[Testing re36::string]\n");
    re36::string str1 = "Hello";
    re36::string str2 = " World";
    re36::string result = str1 + str2 + " From User-Space Vector/String API!";
    
    printf("   Concat result: %s\n", result.c_str());
    printf("   String size: %d bytes\n\n", result.size());

    printf("Boot completed! System idling.\n");

    while (true) {
        asm volatile("hlt");
    }
}
