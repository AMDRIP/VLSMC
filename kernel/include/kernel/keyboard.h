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

    // Блокирующее чтение символа из буфера (ждет нажатия)
    static char get_char();

    // Неблокирующее чтение (возвращает 0, если буфер пуст)
    static char get_char_nonblocking();

    // Проверка статуса модификаторов
    static bool is_shift_pressed() { return shift_pressed_; }
    static bool is_ctrl_pressed()  { return ctrl_pressed_; }
    static bool is_alt_pressed()   { return alt_pressed_; }

private:
    static void process_scancode(uint8_t scancode);
    
    // Статус модификаторов
    static bool shift_pressed_;
    static bool ctrl_pressed_;
    static bool alt_pressed_;
    static bool caps_lock_active_;
    
    static int current_layout_;

    static int kbd_channel_id_;

    // Кольцевой буфер для символов (256 байт)
    static char char_buffer_[256];
    static int buffer_head_;
    static int buffer_tail_;
    static bool extended_key_;
};

} // namespace re36
