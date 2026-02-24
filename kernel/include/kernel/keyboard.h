#pragma once

#include <stdint.h>
#include <stddef.h>

namespace re36 {

// Структура для представления нажатия клавиши
struct KeyEvent {
    char ascii;          // Раскодированный ASCII символ ('A', '1', ' ' и т.д.)
    uint8_t scancode;    // Исходный аппаратный скан-код
    bool pressed;        // true если клавиша нажата, false если отпущена
};

class KeyboardDriver {
public:
    static void init();

    // Вызывается из idt.cpp при каждом прерывании IRQ1 (IRQ 33)
    static void handle_interrupt();

    // Получить последний считанный символ (для тестирования)
    static char get_last_char();

private:
    static void process_scancode(uint8_t scancode);
    
    // Статус специальных клавиш
    static bool shift_pressed_;
    static bool caps_lock_active_;
    
    // Буфер (в будущем можно сделать кольцевой буфер очереди событий)
    static char last_char_;
};

} // namespace re36
