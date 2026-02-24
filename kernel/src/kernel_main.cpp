#include <stdint.h>
#include <stddef.h>
#include "kernel/idt.h"
#include "kernel/pmm.h"
#include "kernel/pic.h"
#include "kernel/kmalloc.h"
#include "kernel/vector.h"
#include "kernel/string.h"
#include "kernel/keyboard.h"
#include "libc.h" // Для printf

extern "C" void kernel_main() {
    // 1. Инициализируем прерывания (IDT)
    re36::init_idt();

    // 2. Перемапливаем PIC
    re36::pic_remap(0x20, 0x28);

    // 3. Инициализация Physical Memory Manager
    re36::PhysicalMemoryManager::init(0x20000, 32 * 1024 * 1024);
    re36::PhysicalMemoryManager::set_region_free(0x100000, 31 * 1024 * 1024);

    // 4. Инициализация Кучи (Heap) 
    re36::kmalloc_init();

    // 5. Инициализация драйвера клавиатуры
    re36::KeyboardDriver::init();

    // Включаем аппаратные прерывания после настройки обработчиков
    asm volatile("sti");

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
    printf("-> Keyboard Driver (Ring 0) Loaded via IRQ1\n");
    printf("-> Interrupts Enabled (STI)\n\n");

    printf("Advanced Keyboard Driver Demo (Ring 0)\n");
    printf("- Type text to see it appear.\n");
    printf("- Press Alt+Shift to switch Layouts (QWERTY <-> Dvorak).\n");
    printf("- Try Ctrl+C or F1-F10 keys.\n\n");
    printf("> ");

    // Тестовая мини-оболочка
    re36::string input_buffer;

    while (true) {
        char c = getchar(); // Блокирующий вызов (ждет прерывания)
        
        // Обработка ввода (эхо-вывод уже работает внутри keyboard.cpp, 
        // но здесь мы собираем строку для анализа команд)
        if (c == '\n') {
            if (input_buffer == "hello") {
                printf("Hello to you too, Kernel Hacker!\n> ");
            } else if (input_buffer == "clear") {
                // Очистка экрана (грязный хак для демо)
                for (int i = 0; i < 80 * 25; i++) {
                    vga_buffer[i] = (uint16_t(' ') | (0x1F << 8)); 
                }
                // Сброс координат putchar
                printf("\n> "); 
            } else if (input_buffer.size() > 0) {
                printf("Unknown command: %s\n> ", input_buffer.c_str());
            } else {
                printf("> ");
            }
            input_buffer = "";
        } else if (c == '\b') {
            if (input_buffer.size() > 0) {
                // Удаляем последний символ из буфера (это не очень эффективно в re36::string, но для демо сойдет)
                input_buffer[input_buffer.size() - 1] = '\0';
                // TODO: Добавить pop_back() в re36::string
            }
        } else if (c >= 32 && c <= 126) {
            // Добавляем печатные символы
            char temp[2] = {c, '\0'};
            input_buffer += temp;
        }
    }
}
