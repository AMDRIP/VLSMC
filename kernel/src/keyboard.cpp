#include "kernel/keyboard.h"
#include "kernel/pic.h" // Для inb
#include "kernel/event_channel.h"
#include "libc.h"       // Для printf/putchar

namespace re36 {

bool KeyboardDriver::shift_pressed_ = false;
bool KeyboardDriver::ctrl_pressed_ = false;
bool KeyboardDriver::alt_pressed_ = false;
bool KeyboardDriver::caps_lock_active_ = false;

int KeyboardDriver::current_layout_ = 0; // 0: QWERTY, 1: Dvorak
int KeyboardDriver::kbd_channel_id_ = -1;

char KeyboardDriver::char_buffer_[256];
int KeyboardDriver::buffer_head_ = 0;
int KeyboardDriver::buffer_tail_ = 0;
bool KeyboardDriver::extended_key_ = false;

// Американская раскладка QWERTY (Scancode Set 1) - Нажатия 
static const char kbd_us_qwerty[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']', '\n',
    0, // Left Ctrl (29)
    'a','s','d','f','g','h','j','k','l',';','\'','`',
    0, // Left Shift (42)
    '\\','z','x','c','v','b','n','m',',','.','/',
    0, // Right Shift (54)
    '*',
    0, // Alt (56)
    ' ', // Space (57)
    0, // CapsLock (58)
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // F1-F10 (59-68)
    0, // NumLock
    0, // ScrollLock
    '7','8','9','-','4','5','6','+','1','2','3','0','.'
};

static const char kbd_us_qwerty_shift[128] = {
    0,  27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}', '\n',
    0, // Left Ctrl
    'A','S','D','F','G','H','J','K','L',':','"','~',
    0, // Left Shift
    '|','Z','X','C','V','B','N','M','<','>','?',
    0, // Right Shift
    '*',
    0, // Alt
    ' ', // Space
    0, // CapsLock
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // F1-F10
    0, 0,
    '7','8','9','-','4','5','6','+','1','2','3','0','.'
};

// Пример Dvorak раскладки
static const char kbd_us_dvorak[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','[',']', '\b',
    '\t','\'',',','.','p','y','f','g','c','r','l','/','=', '\n',
    0, // Left Ctrl
    'a','o','e','u','i','d','h','t','n','s','-','`',
    0, // Left Shift
    '\\',';','q','j','k','x','b','m','w','v','z',
    0, // Right Shift
    '*',
    0, // Alt
    ' ', // Space
    0, // CapsLock
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // F1-F10
    0, 0,
    '7','8','9','-','4','5','6','+','1','2','3','0','.'
};

static const char kbd_us_dvorak_shift[128] = {
    0,  27, '!','@','#','$','%','^','&','*','(',')','{','}', '\b',
    '\t','"','<','>','P','Y','F','G','C','R','L','?','+', '\n',
    0, // Left Ctrl
    'A','O','E','U','I','D','H','T','N','S','_','~',
    0, // Left Shift
    '|',':','Q','J','K','X','B','M','W','V','Z',
    0, // Right Shift
    '*',
    0, // Alt
    ' ', // Space
    0, // CapsLock
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // F1-F10
    0, 0,
    '7','8','9','-','4','5','6','+','1','2','3','0','.'
};

void KeyboardDriver::init() {
    shift_pressed_ = false;
    ctrl_pressed_ = false;
    alt_pressed_ = false;
    caps_lock_active_ = false;
    current_layout_ = 0;
    buffer_head_ = 0;
    buffer_tail_ = 0;
    extended_key_ = false;
    kbd_channel_id_ = EventSystem::create_channel("kbd");
}

void KeyboardDriver::handle_interrupt() {
    uint8_t scancode = inb(0x60);
    process_scancode(scancode);
}

void KeyboardDriver::process_scancode(uint8_t scancode) {
    if (scancode == 0xE0) {
        extended_key_ = true;
        return;
    }

    if (extended_key_) {
        extended_key_ = false;
        if (scancode & 0x80) return;

        char special = 0;
        if (scancode == 0x48) special = (char)0x80;
        if (scancode == 0x50) special = (char)0x81;

        if (special) {
            int next_head = (buffer_head_ + 1) % 256;
            if (next_head != buffer_tail_) {
                char_buffer_[buffer_head_] = special;
                buffer_head_ = next_head;
                EventSystem::push(kbd_channel_id_, 1);
            }
        }
        return;
    }

    bool is_release = (scancode & 0x80) != 0;
    uint8_t make_code = scancode & 0x7F;

    // Модификаторы
    if (make_code == 0x2A || make_code == 0x36) { // Shift
        shift_pressed_ = !is_release;
        return;
    }
    if (make_code == 0x1D) { // Ctrl
        ctrl_pressed_ = !is_release;
        return;
    }
    if (make_code == 0x38) { // Alt
        alt_pressed_ = !is_release;
        return;
    }

    if (make_code == 0x3A && !is_release) { // CapsLock
        caps_lock_active_ = !caps_lock_active_;
        return;
    }

    // Горячие клавиши переключения раскладки (Alt + Shift)
    if (alt_pressed_ && shift_pressed_ && !is_release) {
        current_layout_ = (current_layout_ + 1) % 2; // Переключаем 0 -> 1 -> 0
        printf("\n[Layout Switched to %s]\n", current_layout_ == 0 ? "QWERTY" : "Dvorak");
        return;
    }

    // Обработка функциональных клавиш F1 (0x3B) - F10 (0x44)
    if (make_code >= 0x3B && make_code <= 0x44 && !is_release) {
        printf("\n[F%d key pressed]\n", make_code - 0x3B + 1);
        return;
    }

    // Игнорируем обычное отпускание клавиш
    if (is_release) return;

    // Перевод в ASCII
    if (make_code < 128) {
        char ascii = 0;
        
        bool is_letter = false;
        
        // Выбор таблицы раскладки
        const char* map_normal = current_layout_ == 0 ? kbd_us_qwerty : kbd_us_dvorak;
        const char* map_shift  = current_layout_ == 0 ? kbd_us_qwerty_shift : kbd_us_dvorak_shift;

        char base_char = map_normal[make_code];
        if (base_char >= 'a' && base_char <= 'z') is_letter = true;
        
        if (shift_pressed_) {
            ascii = map_shift[make_code];
            if (caps_lock_active_ && is_letter) ascii = map_normal[make_code];
        } else {
            ascii = map_normal[make_code];
            if (caps_lock_active_ && is_letter) ascii = map_shift[make_code];
        }

        if (ascii != 0) {
            if (ctrl_pressed_ && is_letter) {
                ascii = ascii - 'a' + 1;
            }

            int next_head = (buffer_head_ + 1) % 256;
            if (next_head != buffer_tail_) {
                char_buffer_[buffer_head_] = ascii;
                buffer_head_ = next_head;
                EventSystem::push(kbd_channel_id_, 1);
            }
        }
    }
}

char KeyboardDriver::get_char_nonblocking() {
    if (buffer_head_ == buffer_tail_) {
        return 0; // Пусто
    }
    char c = char_buffer_[buffer_tail_];
    buffer_tail_ = (buffer_tail_ + 1) % 256;
    return c;
}

char KeyboardDriver::get_char() {
    char c = 0;
    while ((c = get_char_nonblocking()) == 0) {
        asm volatile("sti; hlt; cli");
    }
    asm volatile("sti");
    return c;
}

} // namespace re36
