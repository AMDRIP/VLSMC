#include "libc.h"
#include "kernel/keyboard.h"
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

// Управление VGA
static int cursor_x = 0;
static int cursor_y = 15; // Начинаем писать под логами загрузки
volatile uint16_t* vga_buffer = (volatile uint16_t*)0xB8000;

void putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            vga_buffer[cursor_y * 80 + cursor_x] = (uint16_t(' ') | (0x0F << 8)); 
        } else if (cursor_y > 0) {
            cursor_y--;
            cursor_x = 79;
            vga_buffer[cursor_y * 80 + cursor_x] = (uint16_t(' ') | (0x0F << 8));
        }
    } else {
        vga_buffer[cursor_y * 80 + cursor_x] = (uint16_t((unsigned char)c) | (0x0F << 8)); // Белый текст
        cursor_x++;
    }

    if (cursor_x >= 80) {
        cursor_x = 0;
        cursor_y++;
    }
    if (cursor_y >= 25) {
        // Простой скроллинг: сдвигаем все строки вверх, очищаем последнюю
        for (int y = 1; y < 25; y++) {
            for (int x = 0; x < 80; x++) {
                vga_buffer[(y - 1) * 80 + x] = vga_buffer[y * 80 + x];
            }
        }
        for (int x = 0; x < 80; x++) {
            vga_buffer[24 * 80 + x] = (uint16_t(' ') | (0x0F << 8));
        }
        cursor_y = 24;
    }
}

static void print_uint(unsigned int val, int base) {
    if (val == 0) {
        putchar('0');
        return;
    }

    char buf[32];
    int i = 0;
    while (val > 0) {
        uint32_t rem = val % base;
        if (rem < 10) {
            buf[i++] = '0' + rem;
        } else {
            buf[i++] = 'A' + (rem - 10);
        }
        val /= base;
    }

    for (int j = i - 1; j >= 0; j--) {
        putchar(buf[j]);
    }
}

static void print_int(int val) {
    if (val < 0) {
        putchar('-');
        val = -val;
    }
    print_uint(val, 10);
}

void printf(const char* format, ...) {
    va_list args;
    va_start(args, format);

    while (*format) {
        if (*format == '%') {
            format++;
            switch (*format) {
                case 'd':
                    print_int(va_arg(args, int));
                    break;
                case 'x':
                    print_uint(va_arg(args, unsigned int), 16);
                    break;
                case 'X':
                    print_uint(va_arg(args, unsigned int), 16);
                    break;
                case 'u':
                    print_uint(va_arg(args, unsigned int), 10);
                    break;
                case 's': {
                    const char* str = va_arg(args, const char*);
                    if (!str) str = "(null)";
                    while (*str) {
                        putchar(*str++);
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
                    putchar(*format);
                    break;
            }
        } else {
            putchar(*format);
        }
        format++;
    }

    va_end(args);
}

} // extern "C"
