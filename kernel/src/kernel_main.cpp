#include <stdint.h>
#include <stddef.h>
#include "kernel/idt.h"
#include "kernel/pmm.h"
#include "kernel/pic.h"
#include "kernel/kmalloc.h"

extern "C" void kernel_main() {
    // 1. Инициализируем прерывания (IDT)
    re36::init_idt();

    // 2. Перемапливаем PIC, чтобы аппаратные IRQ (0-15) приходили на 32-47 прерывания
    re36::pic_remap(0x20, 0x28);

    // 3. Включаем аппаратные прерывания (STI)
    asm volatile("sti");

    // 4. Инициализация Physical Memory Manager (ОЗУ = 32 МБ)
    re36::PhysicalMemoryManager::init(0x20000, 32 * 1024 * 1024);

    // Память от 0 до 1 МБ зарезервирована (BIOS, VGA, ядро).
    // Помечаем свободными адреса от 1 МБ до 32 МБ.
    re36::PhysicalMemoryManager::set_region_free(0x100000, 31 * 1024 * 1024);

    // 5. Инициализация Кучи (Heap) поверх PMM
    re36::kmalloc_init();

    // 6. Тест оператора new (динамическое выделение памяти)
    int* dynamic_var = new int(12345);

    // В Protected Mode область видеопамяти VGA начинается с адреса 0xB8000
    volatile uint16_t* vga_buffer = (volatile uint16_t*)0xB8000;
    
    // Очистим экран синим фоном
    for (int i = 0; i < 80 * 25; i++) {
        vga_buffer[i] = (uint16_t(' ') | (0x1F << 8)); // 1 = Синий фон, F = Белый текст
    }

    const char* message = "Hello, Bare-Metal! PMM and Heap Active.";
    int index = 0;
    
    // Пишем сообщение поверх синего фона
    while (message[index] != '\0') {
        vga_buffer[index] = (uint16_t(message[index]) | (0x1F << 8));
        index++;
    }

    // Выведем физический адрес выделенной кучи в HEX
    const char hex[] = "0123456789ABCDEF";
    uint32_t addr = (uint32_t)dynamic_var;
    vga_buffer[160] = (uint16_t('N') | (0x0A << 8)); // Зеленый текст
    vga_buffer[161] = (uint16_t('E') | (0x0A << 8));
    vga_buffer[162] = (uint16_t('W') | (0x0A << 8));
    vga_buffer[163] = (uint16_t(' ') | (0x0A << 8));
    vga_buffer[164] = (uint16_t('P') | (0x0A << 8));
    vga_buffer[165] = (uint16_t('T') | (0x0A << 8));
    vga_buffer[166] = (uint16_t('R') | (0x0A << 8));
    vga_buffer[167] = (uint16_t(':') | (0x0A << 8));
    vga_buffer[168] = (uint16_t('0') | (0x0A << 8));
    vga_buffer[169] = (uint16_t('x') | (0x0A << 8));
    
    for (int i = 0; i < 8; i++) {
        uint8_t nibble = (addr >> (28 - i * 4)) & 0x0F;
        vga_buffer[170 + i] = (uint16_t(hex[nibble]) | (0x0A << 8));
    }

    // Очистим память, чтобы проверить kfree
    delete dynamic_var;

    // Бесконечный цикл ОС
    while (true) {
        asm volatile("hlt"); // Усыпляем процессор до следующего прерывания, экономим CPU
    }
}
