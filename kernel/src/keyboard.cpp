#include "kernel/keyboard.h"
#include "kernel/pic.h" // Для inb
#include "libc.h"       // Для printf/putchar

namespace re36 {

bool KeyboardDriver::shift_pressed_ = false;
bool KeyboardDriver::caps_lock_active_ = false;
char KeyboardDriver::last_char_ = 0;

// Американская раскладка QWERTY (Scancode Set 1) - Нажатия (Make codes)
static const char kbd_us_macromap[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b', // 0-14
    '\t','q','w','e','r','t','y','u','i','o','p','[',']', '\n', // 15-28
    0, // Left Ctrl
    'a','s','d','f','g','h','j','k','l',';','\'','`',   // 30-41
    0, // Left Shift
    '\\','z','x','c','v','b','n','m',',','.','/',       // 43-53
    0, // Right Shift
    '*',
    0, // Alt
    ' ', // Space
    0, // CapsLock
    // Функциональные клавиши F1-F10
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, // NumLock
    0, // ScrollLock
    // Numpad
    '7','8','9','-','4','5','6','+','1','2','3','0','.'
};

// Американская раскладка с зажатым Shift
static const char kbd_us_macromap_shift[128] = {
    0,  27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b', // 0-14
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}', '\n', // 15-28
    0, // Left Ctrl
    'A','S','D','F','G','H','J','K','L',':','"','~',   // 30-41
    0, // Left Shift
    '|','Z','X','C','V','B','N','M','<','>','?',       // 43-53
    0, // Right Shift
    '*',
    0, // Alt
    ' ', // Space
    0, // CapsLock
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0,
    '7','8','9','-','4','5','6','+','1','2','3','0','.'
};

void KeyboardDriver::init() {
    shift_pressed_ = false;
    caps_lock_active_ = false;
    last_char_ = 0;
    // В будущем тут можно отправить команды сброса клаве через OUT 0x60
}

void KeyboardDriver::handle_interrupt() {
    // Читаем 1 байт из порта данных контроллера PS/2 (0x60)
    uint8_t scancode = inb(0x60);
    process_scancode(scancode);
}

void KeyboardDriver::process_scancode(uint8_t scancode) {
    // В Scancode Set 1, если установлен старший бит (0x80), это Release код (отпускание)
    bool is_release = (scancode & 0x80) != 0;
    uint8_t make_code = scancode & 0x7F;

    // Проверка Shift
    if (make_code == 0x2A || make_code == 0x36) { // Left Shift (0x2A), Right Shift (0x36)
        shift_pressed_ = !is_release; // Если нажали - true, отпустили - false
        return;
    }

    // Проверка Caps Lock
    if (make_code == 0x3A && !is_release) { // CapsLock (0x3A) обрабатывается только при нажатии
        caps_lock_active_ = !caps_lock_active_;
        return;
    }

    // Мы игнорируем события "отпускания" клавиш для печати текста
    if (is_release) {
        return;
    }

    // Перевод в ASCII
    if (make_code < 128) {
        char ascii = 0;
        
        // Логика регистра букв: инвертируется, если нажат Shift ИЛИ включен CapsLock
        // Но Shift инвертирует еще и цифры в символы.
        bool is_letter = (make_code >= 0x10 && make_code <= 0x19) || // q-p
                         (make_code >= 0x1E && make_code <= 0x26) || // a-l
                         (make_code >= 0x2C && make_code <= 0x32);   // z-m
        
        if (shift_pressed_) {
            ascii = kbd_us_macromap_shift[make_code];
            // Особый случай: CapsLock + Shift = маленькая буква
            if (caps_lock_active_ && is_letter) {
                ascii = kbd_us_macromap[make_code];
            }
        } else {
            ascii = kbd_us_macromap[make_code];
            // Просто CapsLock
            if (caps_lock_active_ && is_letter) {
                ascii = kbd_us_macromap_shift[make_code];
            }
        }

        if (ascii != 0) {
            last_char_ = ascii;
            // Пока что мы просто отправляем в putchar для демонстрации работы!
            putchar(ascii);
        }
    }
}

char KeyboardDriver::get_last_char() {
    char c = last_char_;
    last_char_ = 0;
    return c;
}

} // namespace re36
