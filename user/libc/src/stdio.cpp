#include "stdio.h"
#include "sys/syscall.h"
#include "errno.h"

static char stdout_buf[1024];
static size_t stdout_pos = 0;

static void fflush_stdout() {
    if (stdout_pos > 0) {
        syscall(SYS_PRINT, (long)stdout_buf, (long)stdout_pos);
        stdout_pos = 0;
    }
}

int putchar(int c) {
    if (stdout_pos >= sizeof(stdout_buf)) {
        fflush_stdout();
    }
    stdout_buf[stdout_pos++] = (char)c;
    if (c == '\n') {
        fflush_stdout();
    }
    return (unsigned char)c;
}

int puts(const char* s) {
    int count = 0;
    while (*s) {
        putchar(*s++);
        count++;
    }
    putchar('\n');
    return count + 1;
}

static void print_int(int val) {
    if (val == 0) {
        putchar('0');
        return;
    }
    if (val < 0) {
        putchar('-');
        val = -val;
    }
    char buf[12];
    int i = 11;
    buf[i] = '\0';
    while (val > 0) {
        i--;
        buf[i] = '0' + (val % 10);
        val /= 10;
    }
    while (buf[i]) {
        putchar(buf[i++]);
    }
}

static void print_hex(unsigned int val, int uppercase) {
    if (val == 0) {
        putchar('0');
        return;
    }
    char buf[9];
    int i = 8;
    buf[i] = '\0';
    while (val > 0) {
        i--;
        int digit = val % 16;
        if (digit < 10) {
            buf[i] = '0' + digit;
        } else {
            buf[i] = (uppercase ? 'A' : 'a') + (digit - 10);
        }
        val /= 16;
    }
    while (buf[i]) {
        putchar(buf[i++]);
    }
}

static void print_ptr(void* ptr) {
    putchar('0');
    putchar('x');
    unsigned int val = (unsigned int)ptr;
    if (val == 0) {
        putchar('0');
        return;
    }
    char buf[9];
    for (int i = 0; i < 8; i++) {
        int digit = (val >> ((7 - i) * 4)) & 0xF;
        if (digit < 10) {
            buf[i] = '0' + digit;
        } else {
            buf[i] = 'a' + (digit - 10);
        }
    }
    buf[8] = '\0';
    int start = 0;
    while (buf[start] == '0' && start < 7) {
        start++;
    }
    while (buf[start]) {
        putchar(buf[start++]);
    }
}

int printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    int count = 0;

    int in_format = 0;
    while (*format) {
        if (!in_format) {
            if (*format == '%') {
                in_format = 1;
            } else {
                putchar(*format);
                count++;
            }
        } else {
            switch (*format) {
                case 's': {
                    const char* str = va_arg(args, const char*);
                    if (!str) str = "(null)";
                    while (*str) {
                        putchar(*str++);
                        count++;
                    }
                    break;
                }
                case 'd': {
                    int val = va_arg(args, int);
                    print_int(val);
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    putchar(c);
                    count++;
                    break;
                }
                case 'x': {
                    unsigned int val = va_arg(args, unsigned int);
                    print_hex(val, 0);
                    break;
                }
                case 'X': {
                    unsigned int val = va_arg(args, unsigned int);
                    print_hex(val, 1);
                    break;
                }
                case 'p': {
                    void* ptr = va_arg(args, void*);
                    print_ptr(ptr);
                    break;
                }
                case '%': {
                    putchar('%');
                    count++;
                    break;
                }
                default: {
                    putchar('%');
                    putchar(*format);
                    count += 2;
                    break;
                }
            }
            in_format = 0;
        }
        format++;
    }

    va_end(args);
    fflush_stdout();
    return count;
}

int getchar(void) {
    long ret = syscall(SYS_GETCHAR);
    if (ret < 0) return EOF;
    return (int)ret;
}

char* gets_s(char* buffer, size_t size) {
    if (!buffer || size == 0) {
        errno = 22;
        return nullptr;
    }

    size_t i = 0;
    while (i < size - 1) {
        int c = getchar();
        if (c == EOF) {
            if (i == 0) return nullptr;
            break;
        }

        if (c == '\n' || c == '\r') {
            putchar('\n');
            break;
        }

        if (c == '\b' || c == 0x7F) {
            if (i > 0) {
                i--;
                putchar('\b');
                putchar(' ');
                putchar('\b');
            }
        } else {
            buffer[i++] = (char)c;
            putchar(c);
        }
    }

    buffer[i] = '\0';
    return buffer;
}
