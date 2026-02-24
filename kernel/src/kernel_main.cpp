#include <stdint.h>
#include <stddef.h>
#include "kernel/idt.h"
#include "kernel/pmm.h"
#include "kernel/pic.h"
#include "kernel/kmalloc.h"
#include "kernel/vector.h"

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

    volatile uint16_t* vga_buffer = (volatile uint16_t*)0xB8000;
    for (int i = 0; i < 80 * 25; i++) {
        vga_buffer[i] = (uint16_t(' ') | (0x1F << 8)); // Синий фон
    }

    const char* message = "Hello, Bare-Metal! Testing re36::vector...";
    int index = 0;
    while (message[index] != '\0') {
        vga_buffer[index] = (uint16_t(message[index]) | (0x1F << 8));
        index++;
    }

    // 6. Тест re36::vector
    re36::vector<int> my_vector;
    my_vector.push_back(42);
    my_vector.push_back(1337);
    my_vector.push_back(2026);

    // Выведем значения вектора на экран
    const char hex[] = "0123456789ABCDEF";
    for (size_t i = 0; i < my_vector.size(); i++) {
        uint32_t val = static_cast<uint32_t>(my_vector[i]);
        
        int offset = 160 + (i * 12);
        vga_buffer[offset] = (uint16_t('V') | (0x0A << 8));
        vga_buffer[offset+1] = (uint16_t(':') | (0x0A << 8));
        
        for (int j = 0; j < 8; j++) {
            uint8_t nibble = (val >> (28 - j * 4)) & 0x0F;
            vga_buffer[offset + 2 + j] = (uint16_t(hex[nibble]) | (0x0A << 8));
        }
    }

    while (true) {
        asm volatile("hlt");
    }
}
