#include "stdio.h"
#include "sys/syscall.h"
#include "errno.h"
#include "malloc.h"

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

static void print_float(double val, int precision) {
    if (precision <= 0) precision = 6;

    if (val < 0.0) {
        putchar('-');
        val = -val;
    }

    unsigned int int_part = (unsigned int)val;
    double frac = val - (double)int_part;

    if (int_part == 0) {
        putchar('0');
    } else {
        char buf[12];
        int i = 11;
        buf[i] = '\0';
        while (int_part > 0) {
            i--;
            buf[i] = '0' + (int)(int_part % 10);
            int_part /= 10;
        }
        while (buf[i]) {
            putchar(buf[i++]);
        }
    }

    putchar('.');

    for (int i = 0; i < precision; i++) {
        frac *= 10.0;
        int digit = (int)frac;
        putchar('0' + digit);
        frac -= digit;
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
                case 'f': {
                    double val = va_arg(args, double);
                    print_float(val, 6);
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

FILE* fopen(const char* filename, const char* mode) {
    if (!filename || !mode) return nullptr;

    int mode_flag = 0;
    if (mode[0] == 'r') mode_flag = FMODE_READ;
    else if (mode[0] == 'w') mode_flag = FMODE_WRITE;
    else return nullptr;

    long fd = syscall(SYS_FOPEN, (long)filename, (long)mode_flag);
    if (fd == -1) return nullptr;

    FILE* f = (FILE*)malloc(sizeof(FILE));
    if (!f) return nullptr;

    f->fd = (int)fd;
    f->mode = mode_flag;
    f->eof = 0;
    f->error = 0;
    return f;
}

int fclose(FILE* stream) {
    if (!stream) return -1;
    long ret = syscall(SYS_FCLOSE, (long)stream->fd);
    free(stream);
    return (int)ret;
}

size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    if (!ptr || !stream || size == 0 || nmemb == 0) return 0;
    size_t total = size * nmemb;
    long bytes = syscall(SYS_FREAD, (long)stream->fd, (long)ptr, (long)total);
    if (bytes <= 0) {
        stream->eof = 1;
        return 0;
    }
    return (size_t)bytes / size;
}

size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream) {
    if (!ptr || !stream || size == 0 || nmemb == 0) return 0;
    size_t total = size * nmemb;
    long bytes = syscall(SYS_FWRITE, (long)stream->fd, (long)ptr, (long)total);
    if (bytes <= 0) {
        stream->error = 1;
        return 0;
    }
    return (size_t)bytes / size;
}

int feof(FILE* stream) {
    if (!stream) return 1;
    return stream->eof;
}

int ferror(FILE* stream) {
    if (!stream) return 1;
    return stream->error;
}
