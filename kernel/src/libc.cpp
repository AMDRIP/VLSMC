#include "libc.h"
#include "kernel/keyboard.h"
#include "kernel/pic.h"
#include "kernel/vga.h"
#include "kernel/bga.h"
#include <stdint.h>
#include <stdarg.h>

extern "C" {

char getchar() {
    return re36::KeyboardDriver::get_char();
}

void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

void* memset(void* s, int c, size_t n) {
    uint8_t* p = (uint8_t*)s;
    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)c;
    }
    return s;
}

size_t strlen(const char* s) {
    size_t len = 0;
    while (s[len]) {
        len++;
    }
    return len;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int atoi(const char* str) {
    int res = 0;
    int sign = 1;

    while (*str == ' ' || *str == '\t' || *str == '\n') str++;

    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }

    while (*str >= '0' && *str <= '9') {
        res = res * 10 + (*str - '0');
        str++;
    }

    return res * sign;
}

// Управление терминалом
static int cursor_x = 0;
static int cursor_y = 15;
static uint8_t term_fg = 0x0F;
static uint8_t term_bg = 0x00;
static uint8_t term_color = 0x0F;

static uint32_t bga_fg = 0xFFFFFF; // White
static uint32_t bga_bg = 0x000000; // Black

volatile uint16_t* vga_buffer = (volatile uint16_t*)0xB8000;

// Simple VGA color to 32-bit hex mapping for compatibility
static uint32_t vga_to_hex(uint8_t color) {
    switch (color & 0x0F) {
        case 0: return 0x000000; // Black
        case 1: return 0x0000AA; // Blue
        case 2: return 0x00AA00; // Green
        case 3: return 0x00AAAA; // Cyan
        case 4: return 0xAA0000; // Red
        case 5: return 0xAA00AA; // Magenta
        case 6: return 0xAA5500; // Brown
        case 7: return 0xAAAAAA; // Light Gray
        case 8: return 0x555555; // Dark Gray
        case 9: return 0x5555FF; // Light Blue
        case 10: return 0x55FF55; // Light Green
        case 11: return 0x55FFFF; // Light Cyan
        case 12: return 0xFF5555; // Light Red
        case 13: return 0xFF55FF; // Light Magenta
        case 14: return 0xFFFF55; // Yellow
        case 15: return 0xFFFFFF; // White
        default: return 0xFFFFFF;
    }
}

void set_color(uint8_t fg, uint8_t bg) {
    term_fg = fg;
    term_bg = bg;
    term_color = fg | (bg << 4);
    bga_fg = vga_to_hex(fg);
    bga_bg = vga_to_hex(bg);
}

static void update_vga_cursor(int x, int y) {
    uint16_t pos = y * 80 + x;
    re36::outb(0x3D4, 0x0F);
    re36::outb(0x3D5, (uint8_t)(pos & 0xFF));
    re36::outb(0x3D4, 0x0E);
    re36::outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

static void draw_bga_cursor(int x, int y, bool draw) {
    uint32_t color = draw ? bga_fg : bga_bg;
    int px_x = x * 8;
    int px_y = y * 8;
    for (int i = 0; i < 8; i++) {
        re36::BgaDriver::put_pixel(px_x + i, px_y + 7, color);
        re36::BgaDriver::put_pixel(px_x + i, px_y + 6, color);
    }
}

#define COM1_PORT 0x3F8
static bool serial_ready = false;

void serial_init() {
    using namespace re36;
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x80);
    outb(COM1_PORT + 0, 0x01);
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x03);
    outb(COM1_PORT + 2, 0xC7);
    outb(COM1_PORT + 4, 0x0B);
    serial_ready = true;
}

static void serial_putchar(char c) {
    if (!serial_ready) return;
    using namespace re36;
    while ((inb(COM1_PORT + 5) & 0x20) == 0);
    outb(COM1_PORT, c);
}

void putchar(char c) {
    if (c == '\n') serial_putchar('\r');
    serial_putchar(c);

    bool is_bga = re36::BgaDriver::is_initialized();
    bool is_gfx = re36::VGA::is_graphics();
    
    int max_x = 80;
    int max_y = 25;
    
    if (is_bga) {
        max_x = re36::BgaDriver::get_width() / 8;
        max_y = re36::BgaDriver::get_height() / 8;
    } else if (is_gfx) {
        max_x = 40;
    }

    if (c == '\n') {
        if (is_bga) draw_bga_cursor(cursor_x, cursor_y, false);
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            if (is_bga) draw_bga_cursor(cursor_x, cursor_y, false);
            cursor_x--;
            if (is_bga) {
                re36::BgaDriver::draw_char(cursor_x * 8, cursor_y * 8, ' ', bga_fg, bga_bg);
            } else if (is_gfx) {
                re36::VGA::draw_char(cursor_x * 8, cursor_y * 8, ' ', term_fg, term_bg);
            } else {
                vga_buffer[cursor_y * 80 + cursor_x] = (uint16_t(' ') | (term_color << 8)); 
            }
        } else if (cursor_y > 0) {
            if (is_bga) draw_bga_cursor(cursor_x, cursor_y, false);
            cursor_y--;
            cursor_x = max_x - 1;
            if (is_bga) {
                re36::BgaDriver::draw_char(cursor_x * 8, cursor_y * 8, ' ', bga_fg, bga_bg);
            } else if (is_gfx) {
                re36::VGA::draw_char(cursor_x * 8, cursor_y * 8, ' ', term_fg, term_bg);
            } else {
                vga_buffer[cursor_y * 80 + cursor_x] = (uint16_t(' ') | (term_color << 8));
            }
        }
    } else {
        if (is_bga) {
            draw_bga_cursor(cursor_x, cursor_y, false);
            re36::BgaDriver::draw_char(cursor_x * 8, cursor_y * 8, c, bga_fg, bga_bg);
        } else if (is_gfx) {
            re36::VGA::draw_char(cursor_x * 8, cursor_y * 8, c, term_fg, term_bg);
        } else {
            vga_buffer[cursor_y * 80 + cursor_x] = (uint16_t((unsigned char)c) | (term_color << 8));
        }
        cursor_x++;
    }

    if (cursor_x >= max_x) {
        cursor_x = 0;
        cursor_y++;
    }
    
    if (cursor_y >= max_y) {
        if (is_bga) {
            re36::BgaDriver::scroll(1, bga_bg);
            cursor_y = max_y - 1;
        } else if (is_gfx) {
            re36::VGA::clear(term_bg);
            cursor_y = 0; 
        } else {
            for (int y = 1; y < max_y; y++) {
                for (int x = 0; x < max_x; x++) {
                    vga_buffer[(y - 1) * 80 + x] = vga_buffer[y * 80 + x];
                }
            }
            for (int x = 0; x < max_x; x++) {
                vga_buffer[24 * 80 + x] = (uint16_t(' ') | (term_color << 8));
            }
            cursor_y = max_y - 1;
        }
    }
    
    if (is_bga) {
        draw_bga_cursor(cursor_x, cursor_y, true);
    } else if (!is_gfx) {
        update_vga_cursor(cursor_x, cursor_y);
    }
}

static void print_uint_core(uint32_t val, int base, bool is_signed, bool is_negative, int width, char pad_char, bool left_justify) {
    char buf[32];
    int i = 0;
    
    if (val == 0) {
        buf[i++] = '0';
    } else {
        while (val > 0) {
            uint32_t rem = val % base;
            if (rem < 10) buf[i++] = '0' + rem;
            else buf[i++] = 'A' + (rem - 10);
            val /= base;
        }
    }
    
    int prefix_len = (is_signed && is_negative) ? 1 : 0;
    int pad_len = width - i - prefix_len;
    if (pad_len < 0) pad_len = 0;
    
    if (!left_justify && pad_char == ' ') {
        for (int p = 0; p < pad_len; p++) putchar(' ');
    }
    
    if (is_signed && is_negative) putchar('-');
    
    if (!left_justify && pad_char == '0') {
        for (int p = 0; p < pad_len; p++) putchar('0');
    }
    
    for (int j = i - 1; j >= 0; j--) {
        putchar(buf[j]);
    }
    
    if (left_justify) {
        for (int p = 0; p < pad_len; p++) putchar(' ');
    }
}

static void print_uint(unsigned int val, int base, int width, char pad_char, bool left_justify) {
    print_uint_core(val, base, false, false, width, pad_char, left_justify);
}

static void print_int(int val, int width, char pad_char, bool left_justify) {
    bool is_neg = val < 0;
    uint32_t uval = is_neg ? (uint32_t)-val : (uint32_t)val;
    print_uint_core(uval, 10, true, is_neg, width, pad_char, left_justify);
}

void printf(const char* format, ...) {
    va_list args;
    va_start(args, format);

    while (*format) {
        if (*format == '%') {
            format++;
            
            bool left_justify = false;
            char pad_char = ' ';
            int width = 0;
            
            if (*format == '-') {
                left_justify = true;
                format++;
            }
            if (*format == '0') {
                pad_char = '0';
                format++;
            }
            
            while (*format >= '0' && *format <= '9') {
                width = width * 10 + (*format - '0');
                format++;
            }
            
            switch (*format) {
                case 'd':
                    print_int(va_arg(args, int), width, pad_char, left_justify);
                    break;
                case 'x':
                case 'X':
                    print_uint(va_arg(args, unsigned int), 16, width, pad_char, left_justify);
                    break;
                case 'u':
                    print_uint(va_arg(args, unsigned int), 10, width, pad_char, left_justify);
                    break;
                case 's': {
                    const char* str = va_arg(args, const char*);
                    if (!str) str = "(null)";
                    
                    int len = 0;
                    while (str[len]) len++;
                    
                    int pad_len = width - len;
                    if (pad_len < 0) pad_len = 0;
                    
                    if (!left_justify) {
                        for (int p=0; p<pad_len; p++) putchar(' ');
                    }
                    
                    while (*str) putchar(*str++);
                    
                    if (left_justify) {
                        for (int p=0; p<pad_len; p++) putchar(' ');
                    }
                    break;
                }
                case 'c':
                    putchar((char)va_arg(args, int));
                    break;
                case '%':
                    putchar('%');
                    break;
                default:
                    putchar('%');
                    if (*format) putchar(*format);
                    break;
            }
        } else {
            putchar(*format);
        }
        if (*format) format++;
    }

    va_end(args);
}

} // extern "C"
