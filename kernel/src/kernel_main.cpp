#include <stdint.h>
#include <stddef.h>
#include "kernel/idt.h"
#include "kernel/pic.h"

extern "C" void kernel_main() {
    // 1. Инициализируем прерывания (IDT)
    re36::init_idt();

    // 2. Перемапливаем PIC, чтобы аппаратные IRQ (0-15) приходили на 32-47 прерывания
    re36::pic_remap(0x20, 0x28);

    // 3. Включаем аппаратные прерывания (STI)
    asm volatile("sti");

    // В Protected Mode область видеопамяти VGA начинается с адреса 0xB8000
    volatile uint16_t* vga_buffer = (volatile uint16_t*)0xB8000;
    
    // Очистим экран синим фоном
    for (int i = 0; i < 80 * 25; i++) {
        vga_buffer[i] = (uint16_t(' ') | (0x1F << 8)); // 1 = Синий фон, F = Белый текст
    }

    const char* message = "Hello, Bare-Metal! IDT and PIC initialized. Press any key on your keyboard...";
    int index = 0;
    
    // Пишем сообщение поверх синего фона
    while (message[index] != '\0') {
        vga_buffer[index] = (uint16_t(message[index]) | (0x1F << 8));
        index++;
    }

    // Бесконечный цикл ОС
    while (true) {
        asm volatile("hlt"); // Усыпляем процессор до следующего прерывания, экономим CPU
    }
}
