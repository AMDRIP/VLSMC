#include "stdio.h"
#include "sys/syscall.h"
#include "errno.h"
#include "malloc.h"
#include <sys/mutex.h>

static char stdout_buf[1024];
static size_t stdout_pos = 0;
static mutex_t stdout_lock = 0;

static void _fflush_stdout_unlocked() {
    if (stdout_pos > 0) {
        syscall(SYS_PRINT, (long)stdout_buf, (long)stdout_pos);
        stdout_pos = 0;
    }
}

static int _putchar_unlocked(int c) {
    if (stdout_pos >= sizeof(stdout_buf)) {
        _fflush_stdout_unlocked();
    }
    stdout_buf[stdout_pos++] = (char)c;
    if (c == '\n') {
        _fflush_stdout_unlocked();
    }
    return (unsigned char)c;
}

int putchar(int c) {
    mutex_lock(&stdout_lock);
    int ret = _putchar_unlocked(c);
    mutex_unlock(&stdout_lock);
    return ret;
}

int puts(const char* s) {
    mutex_lock(&stdout_lock);
    int count = 0;
    while (*s) {
        _putchar_unlocked(*s++);
        count++;
    }
    _putchar_unlocked('\n');
    mutex_unlock(&stdout_lock);
    return count + 1;
}

#define putchar _putchar_unlocked
#define fflush_stdout _fflush_stdout_unlocked

static void print_uint_core(unsigned int val, int base, int uppercase, bool is_signed, bool is_negative, int width, char pad_char, bool left_justify) {
    char buf[32];
    int i = 0;
    
    if (val == 0) {
        buf[i++] = '0';
    } else {
        while (val > 0) {
            unsigned int rem = val % base;
            if (rem < 10) buf[i++] = '0' + rem;
            else buf[i++] = (uppercase ? 'A' : 'a') + (rem - 10);
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

static void print_int(int val, int width, char pad_char, bool left_justify) {
    bool is_neg = val < 0;
    unsigned int uval = is_neg ? (unsigned int)-val : (unsigned int)val;
    print_uint_core(uval, 10, 0, true, is_neg, width, pad_char, left_justify);
}

static void print_hex(unsigned int val, int uppercase, int width, char pad_char, bool left_justify) {
    print_uint_core(val, 16, uppercase, false, false, width, pad_char, left_justify);
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
    mutex_lock(&stdout_lock);
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
                case 's': {
                    const char* str = va_arg(args, const char*);
                    if (!str) str = "(null)";
                    
                    int len = 0;
                    while (str[len]) len++;
                    
                    int pad_len = width - len;
                    if (pad_len < 0) pad_len = 0;
                    
                    if (!left_justify) {
                        for (int p=0; p<pad_len; p++) { putchar(' '); count++; }
                    }
                    
                    while (*str) {
                        putchar(*str++);
                        count++;
                    }
                    
                    if (left_justify) {
                        for (int p=0; p<pad_len; p++) { putchar(' '); count++; }
                    }
                    break;
                }
                case 'd':
                case 'i': {
                    int val = va_arg(args, int);
                    print_int(val, width, pad_char, left_justify);
                    break;
                }
                case 'u': {
                    unsigned int val = va_arg(args, unsigned int);
                    print_uint_core(val, 10, 0, false, false, width, pad_char, left_justify);
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
                    print_hex(val, 0, width, pad_char, left_justify);
                    break;
                }
                case 'X': {
                    unsigned int val = va_arg(args, unsigned int);
                    print_hex(val, 1, width, pad_char, left_justify);
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
                    print_float(val, width > 0 ? width : 6); 
                    break;
                }
                default: {
                    putchar('%');
                    if (*format) putchar(*format);
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
    mutex_unlock(&stdout_lock);
    return count;
}
#undef putchar
#undef fflush_stdout

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

void flockfile(FILE* stream) {
    if (stream) mutex_lock((mutex_t*)&stream->lock);
}

void funlockfile(FILE* stream) {
    if (stream) mutex_unlock((mutex_t*)&stream->lock);
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
    if (!f) {
        syscall(SYS_FCLOSE, fd);
        return nullptr;
    }

    f->buffer = (char*)malloc(BUFSIZ);
    if (!f->buffer) {
        syscall(SYS_FCLOSE, fd);
        free(f);
        return nullptr;
    }

    f->fd = (int)fd;
    f->mode = mode_flag;
    f->eof = 0;
    f->error = 0;
    f->buffer_size = BUFSIZ;
    f->buffer_pos = 0;
    f->bytes_in_buf = 0;
    f->lock = 0;
    return f;
}

static int _fflush_unlocked(FILE* stream) {
    if (!stream) return EOF;
    if (stream->mode == FMODE_WRITE && stream->buffer_pos > 0) {
        long bytes = syscall(SYS_FWRITE, (long)stream->fd, (long)stream->buffer, (long)stream->buffer_pos);
        if (bytes <= 0) {
            stream->error = 1;
            return EOF;
        }
    }
    stream->buffer_pos = 0;
    stream->bytes_in_buf = 0;
    return 0;
}

int fflush(FILE* stream) {
    if (!stream) return EOF;
    flockfile(stream);
    int ret = _fflush_unlocked(stream);
    funlockfile(stream);
    return ret;
}

int fclose(FILE* stream) {
    if (!stream) return EOF;
    flockfile(stream);
    int ret = _fflush_unlocked(stream);
    long closed = syscall(SYS_FCLOSE, (long)stream->fd);
    if (stream->buffer) free(stream->buffer);
    funlockfile(stream);
    free(stream);
    return (ret == 0 && closed == 0) ? 0 : EOF;
}

int fgetc(FILE* stream) {
    if (!stream) return EOF;
    flockfile(stream);
    if (stream->eof || stream->error || stream->mode != FMODE_READ) {
        funlockfile(stream);
        return EOF;
    }

    if (stream->buffer_pos >= stream->bytes_in_buf) {
        long bytes = syscall(SYS_FREAD, (long)stream->fd, (long)stream->buffer, (long)stream->buffer_size);
        if (bytes <= 0) {
            stream->eof = 1;
            funlockfile(stream);
            return EOF;
        }
        stream->bytes_in_buf = (size_t)bytes;
        stream->buffer_pos = 0;
    }

    int ret = (unsigned char)stream->buffer[stream->buffer_pos++];
    funlockfile(stream);
    return ret;
}

int fputc(int c, FILE* stream) {
    if (!stream) return EOF;
    flockfile(stream);
    if (stream->error || stream->mode != FMODE_WRITE) {
        funlockfile(stream);
        return EOF;
    }

    if (stream->buffer_pos >= stream->buffer_size) {
        if (_fflush_unlocked(stream) != 0) {
            funlockfile(stream);
            return EOF;
        }
    }

    stream->buffer[stream->buffer_pos++] = (char)c;
    funlockfile(stream);
    return (unsigned char)c;
}

size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    if (!ptr || !stream || size == 0 || nmemb == 0) return 0;
    flockfile(stream);
    if (stream->mode != FMODE_READ) {
        funlockfile(stream);
        return 0;
    }
    
    size_t total = size * nmemb;
    size_t bytes_read = 0;
    char* dest = (char*)ptr;

    while (bytes_read < total) {
        size_t available = stream->bytes_in_buf - stream->buffer_pos;
        if (available > 0) {
            size_t to_copy = total - bytes_read;
            if (to_copy > available) to_copy = available;
            
            for (size_t i = 0; i < to_copy; i++) {
                dest[bytes_read + i] = stream->buffer[stream->buffer_pos + i];
            }
            
            bytes_read += to_copy;
            stream->buffer_pos += to_copy;
        } else {
            long bytes = syscall(SYS_FREAD, (long)stream->fd, (long)stream->buffer, (long)stream->buffer_size);
            if (bytes <= 0) {
                stream->eof = 1;
                break;
            }
            stream->bytes_in_buf = (size_t)bytes;
            stream->buffer_pos = 0;
        }
    }

    funlockfile(stream);
    return bytes_read / size;
}

size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream) {
    if (!ptr || !stream || size == 0 || nmemb == 0) return 0;
    flockfile(stream);
    if (stream->mode != FMODE_WRITE) {
        funlockfile(stream);
        return 0;
    }
    
    size_t total = size * nmemb;
    size_t bytes_written = 0;
    const char* src = (const char*)ptr;

    while (bytes_written < total) {
        size_t space = stream->buffer_size - stream->buffer_pos;
        if (space > 0) {
            size_t to_copy = total - bytes_written;
            if (to_copy > space) to_copy = space;
            
            for (size_t i = 0; i < to_copy; i++) {
                stream->buffer[stream->buffer_pos + i] = src[bytes_written + i];
            }
            
            bytes_written += to_copy;
            stream->buffer_pos += to_copy;
        } else {
            if (_fflush_unlocked(stream) != 0) break;
        }
    }

    funlockfile(stream);
    return bytes_written / size;
}

int feof(FILE* stream) {
    if (!stream) return 1;
    flockfile(stream);
    int ret = stream->eof;
    funlockfile(stream);
    return ret;
}

int ferror(FILE* stream) {
    if (!stream) return 1;
    flockfile(stream);
    int ret = stream->error;
    funlockfile(stream);
    return ret;
}
