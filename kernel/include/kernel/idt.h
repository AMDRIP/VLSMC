#pragma once

#include <stdint.h>

namespace re36 {

// Структура записи в IDT (Interrupt Descriptor Table)
struct __attribute__((packed)) IdtEntry {
    uint16_t isr_low;      // Младшие 16 бит адреса обработчика
    uint16_t kernel_cs;    // Селектор сегмента кода (0x08)
    uint8_t  reserved;     // Всегда 0
    uint8_t  attributes;   // Флаги (Type, DPL, Present)
    uint16_t isr_high;     // Старшие 16 бит адреса обработчика
};

// Указатель на IDT для инструкции lidt
struct __attribute__((packed)) IdtPtr {
    uint16_t limit;        // Размер таблицы - 1
    uint32_t base;         // Базовый адрес таблицы
};

// Структура регистров, сохраняемая в ассемблерном обработчике перед вызовом C++
struct Registers {
    uint32_t ds;                                     // Сегмент данных
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
    uint32_t int_no, err_code;                       // Номер прерывания и код ошибки
    uint32_t eip, cs, eflags, useresp, ss;           // Pushed by the processor automatically.
};

// Инициализация IDT
void init_idt();
void set_idt_gate(int n, uint32_t handler, uint16_t selector, uint8_t flags);

} // namespace re36
