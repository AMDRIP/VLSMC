#pragma once

#include <stdint.h>

namespace re36 {

// Инициализация (переназначение) PIC, чтобы аппаратные прерывания
// (IRQ0-15) начинались с 32 номера в IDT (вместо 8, как по умолчанию в BIOS),
// чтобы не конфликтовать с исключениями процессора (Double Fault и т.д.)
void pic_remap(int offset1 = 0x20, int offset2 = 0x28);

// Отправить сигнал End of Interrupt (EOI) в PIC,
// чтобы контроллер знал, что мы обработали прерывание и готов слать новые
void pic_send_eoi(uint8_t irq);

// Базовые функции для общения с I/O портами x86
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ( "inb %1, %0"
                   : "=a"(ret)
                   : "Nd"(port) );
    return ret;
}

static inline void io_wait(void) {
    outb(0x80, 0); // Простая запись в неиспользуемый порт для задержки
}

} // namespace re36
