#include <stdint.h>
#include <stddef.h>

extern "C" void kernel_main() {
    // В Protected Mode область видеопамяти VGA начинается с адреса 0xB8000
    volatile uint16_t* vga_buffer = (volatile uint16_t*)0xB8000;
    
    // Очистим экран синим фоном
    for (int i = 0; i < 80 * 25; i++) {
        vga_buffer[i] = (uint16_t(' ') | (0x1F << 8)); // 1 = Синий фон, F = Белый текст
    }

    const char* message = "Hello, Bare-Metal! VLSMC OS successfully booted into 32-bit Protected Mode.";
    int index = 0;
    
    // Пишем сообщение поверх синего фона
    while (message[index] != '\0') {
        vga_buffer[index] = (uint16_t(message[index]) | (0x1F << 8));
        index++;
    }

    // Бесконечный цикл, чтобы процессор не выполнял мусор после функции
    while (true) {
        // asm volatile("hlt");
    }
}
