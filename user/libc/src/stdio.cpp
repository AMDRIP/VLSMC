#include "stdio.h"
#include "../../syscalls.h"
#include "errno.h"
#include "malloc.h"
#include <sys/mutex.h>

static char stdout_buf[1024];
static size_t stdout_pos = 0;
static mutex_t stdout_lock = 0;

static void _fflush_stdout_unlocked() {
    if (stdout_pos > 0) {
        syscall2(SYS_PRINT, (long)stdout_buf, (long)stdout_pos);
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

static void print_float(double val, int precision, void (*putc_cb)(char, void*), void* ctx) {
    if (precision <= 0) precision = 6;

    if (val < 0.0) {
        putc_cb('-', ctx);
        val = -val;
    }

    unsigned int int_part = (unsigned int)val;
    double frac = val - (double)int_part;

    if (int_part == 0) {
        putc_cb('0', ctx);
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
            putc_cb(buf[i++], ctx);
        }
    }

    putc_cb('.', ctx);

    for (int i = 0; i < precision; i++) {
        frac *= 10.0;
        int digit = (int)frac;
        putc_cb('0' + digit, ctx);
        frac -= digit;
    }
}

static void print_uint_core(unsigned int val, int base, int uppercase, bool is_signed, bool is_negative, int width, char pad_char, bool left_justify, void (*putc_cb)(char, void*), void* ctx) {
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
        for (int p = 0; p < pad_len; p++) putc_cb(' ', ctx);
    }
    
    if (is_signed && is_negative) putc_cb('-', ctx);
    
    if (!left_justify && pad_char == '0') {
        for (int p = 0; p < pad_len; p++) putc_cb('0', ctx);
    }
    
    for (int j = i - 1; j >= 0; j--) {
        putc_cb(buf[j], ctx);
    }
    
    if (left_justify) {
        for (int p = 0; p < pad_len; p++) putc_cb(' ', ctx);
    }
}

static void print_int(int val, int width, char pad_char, bool left_justify, void (*putc_cb)(char, void*), void* ctx) {
    bool is_neg = val < 0;
    unsigned int uval = is_neg ? (unsigned int)-val : (unsigned int)val;
    print_uint_core(uval, 10, 0, true, is_neg, width, pad_char, left_justify, putc_cb, ctx);
}

static void print_hex(unsigned int val, int uppercase, int width, char pad_char, bool left_justify, void (*putc_cb)(char, void*), void* ctx) {
    print_uint_core(val, 16, uppercase, false, false, width, pad_char, left_justify, putc_cb, ctx);
}

static void print_ptr(void* ptr, void (*putc_cb)(char, void*), void* ctx) {
    putc_cb('0', ctx);
    putc_cb('x', ctx);
    unsigned int val = (unsigned int)ptr;
    if (val == 0) {
        putc_cb('0', ctx);
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
        putc_cb(buf[start++], ctx);
    }
}

int _vcbprintf(void (*putc_cb)(char, void*), void* ctx, const char* format, va_list args) {
    int count = 0;
    int in_format = 0;
    
    while (*format) {
        if (!in_format) {
            if (*format == '%') {
                in_format = 1;
            } else {
                putc_cb(*format, ctx);
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
                        for (int p=0; p<pad_len; p++) { putc_cb(' ', ctx); count++; }
                    }
                    
                    while (*str) {
                        putc_cb(*str++, ctx);
                        count++;
                    }
                    
                    if (left_justify) {
                        for (int p=0; p<pad_len; p++) { putc_cb(' ', ctx); count++; }
                    }
                    break;
                }
                case 'd':
                case 'i': {
                    int val = va_arg(args, int);
                    print_int(val, width, pad_char, left_justify, putc_cb, ctx);
                    break;
                }
                case 'u': {
                    unsigned int val = va_arg(args, unsigned int);
                    print_uint_core(val, 10, 0, false, false, width, pad_char, left_justify, putc_cb, ctx);
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    putc_cb(c, ctx);
                    count++;
                    break;
                }
                case 'x': {
                    unsigned int val = va_arg(args, unsigned int);
                    print_hex(val, 0, width, pad_char, left_justify, putc_cb, ctx);
                    break;
                }
                case 'X': {
                    unsigned int val = va_arg(args, unsigned int);
                    print_hex(val, 1, width, pad_char, left_justify, putc_cb, ctx);
                    break;
                }
                case 'p': {
                    void* ptr = va_arg(args, void*);
                    print_ptr(ptr, putc_cb, ctx);
                    break;
                }
                case '%': {
                    putc_cb('%', ctx);
                    count++;
                    break;
                }
                case 'f': {
                    double val = va_arg(args, double);
                    print_float(val, width > 0 ? width : 6, putc_cb, ctx); 
                    break;
                }
                default: {
                    putc_cb('%', ctx);
                    if (*format) putc_cb(*format, ctx);
                    count += 2;
                    break;
                }
            }
            in_format = 0;
        }
        format++;
    }
    
    return count;
}

// === Formatter Variants ===

static void _stdout_putc_cb(char c, void* ctx) {
    (void)ctx;
    _putchar_unlocked(c);
}

int vprintf(const char* format, va_list ap) {
    mutex_lock(&stdout_lock);
    int ret = _vcbprintf(_stdout_putc_cb, nullptr, format, ap);
    _fflush_stdout_unlocked();
    mutex_unlock(&stdout_lock);
    return ret;
}

int printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vprintf(format, args);
    va_end(args);
    return ret;
}

struct _snprintf_ctx {
    char* buf;
    size_t size;
    size_t pos;
};

static void _snprintf_putc_cb(char c, void* ctx) {
    _snprintf_ctx* pCtx = (_snprintf_ctx*)ctx;
    if (pCtx->pos < pCtx->size - 1) {
        pCtx->buf[pCtx->pos] = c;
    }
    pCtx->pos++;
}

int vsnprintf(char* str, size_t size, const char* format, va_list ap) {
    if (!str && size > 0) return -1;
    _snprintf_ctx ctx = { str, size, 0 };
    int ret = _vcbprintf(_snprintf_putc_cb, &ctx, format, ap);
    if (size > 0) {
        if (ctx.pos < size) str[ctx.pos] = '\0';
        else str[size - 1] = '\0';
    }
    return ret;
}

int vsprintf(char* str, const char* format, va_list ap) {
    return vsnprintf(str, (size_t)-1, format, ap);
}

int sprintf(char* str, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vsprintf(str, format, args);
    va_end(args);
    return ret;
}

int snprintf(char* str, size_t size, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vsnprintf(str, size, format, args);
    va_end(args);
    return ret;
}

static void _fputc_cb(char c, void* ctx) {
    fputc((int)c, (FILE*)ctx);
}



// === Parser Variants ===

static int _isspace(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

int _vcbscanf(int (*getc_cb)(void*), void (*ungetc_cb)(int, void*), void* ctx, const char* format, va_list args) {
    int count = 0;
    int c;

    while (*format) {
        if (_isspace(*format)) {
            while (_isspace(*format)) format++;
            while ((c = getc_cb(ctx)) != EOF && _isspace(c)) { }
            if (c != EOF) ungetc_cb(c, ctx);
            continue;
        }

        if (*format != '%') {
            c = getc_cb(ctx);
            if (c == EOF || c != *format) {
                if (c != EOF) ungetc_cb(c, ctx);
                break;
            }
            format++;
            continue;
        }

        format++; // Skip %
        int suppress = 0;
        if (*format == '*') {
            suppress = 1;
            format++;
        }

        int width = -1;
        while (*format >= '0' && *format <= '9') {
            if (width == -1) width = 0;
            width = width * 10 + (*format - '0');
            format++;
        }

        switch (*format) {
            case 's': {
                while ((c = getc_cb(ctx)) != EOF && _isspace(c)) { }
                if (c == EOF) return count > 0 ? count : EOF;

                char* str = suppress ? nullptr : va_arg(args, char*);
                int read_count = 0;

                while (c != EOF && !_isspace(c) && (width == -1 || read_count < width)) {
                    if (str) str[read_count] = (char)c;
                    read_count++;
                    if (width != -1 && read_count == width) break;
                    c = getc_cb(ctx);
                }
                
                if (c != EOF && (width == -1 || read_count < width)) {
                    ungetc_cb(c, ctx);
                }

                if (str) {
                    str[read_count] = '\0';
                    count++;
                }
                break;
            }
            case 'c': {
                if (width == -1) width = 1;
                char* str = suppress ? nullptr : va_arg(args, char*);
                int read_count = 0;

                while ((width == -1 || read_count < width) && (c = getc_cb(ctx)) != EOF) {
                    if (str) str[read_count] = (char)c;
                    read_count++;
                }

                if (str) count++;
                break;
            }
            case 'd':
            case 'i': {
                while ((c = getc_cb(ctx)) != EOF && _isspace(c)) { }
                if (c == EOF) return count > 0 ? count : EOF;

                int sign = 1;
                if (c == '-') {
                    sign = -1;
                    c = getc_cb(ctx);
                } else if (c == '+') {
                    c = getc_cb(ctx);
                }

                int val = 0;
                int read_count = 0;
                while (c != EOF && c >= '0' && c <= '9' && (width == -1 || read_count < width)) {
                    val = val * 10 + (c - '0');
                    read_count++;
                    c = getc_cb(ctx);
                }

                if (c != EOF) ungetc_cb(c, ctx);

                if (read_count > 0) {
                    if (!suppress) {
                        int* pval = va_arg(args, int*);
                        *pval = val * sign;
                        count++;
                    }
                } else {
                    return count > 0 ? count : EOF;
                }
                break;
            }
            case 'x':
            case 'X': {
                while ((c = getc_cb(ctx)) != EOF && _isspace(c)) { }
                if (c == EOF) return count > 0 ? count : EOF;
                
                unsigned int val = 0;
                int read_count = 0;
                while (c != EOF && (width == -1 || read_count < width)) {
                    int digit = -1;
                    if (c >= '0' && c <= '9') digit = c - '0';
                    else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
                    else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
                    
                    if (digit == -1) break;
                    
                    val = val * 16 + digit;
                    read_count++;
                    c = getc_cb(ctx);
                }

                if (c != EOF) ungetc_cb(c, ctx);

                if (read_count > 0) {
                    if (!suppress) {
                        unsigned int* pval = va_arg(args, unsigned int*);
                        *pval = val;
                        count++;
                    }
                } else {
                    return count > 0 ? count : EOF;
                }
                break;
            }
            default: {
                c = getc_cb(ctx);
                if (c != *format) {
                    if (c != EOF) ungetc_cb(c, ctx);
                    return count;
                }
                break;
            }
        }
        format++;
    }

    return count;
}

struct _sscanf_ctx {
    const char* str;
    size_t pos;
};

static int _sscanf_getc_cb(void* ctx) {
    _sscanf_ctx* pCtx = (_sscanf_ctx*)ctx;
    if (pCtx->str[pCtx->pos] == '\0') return EOF;
    return (unsigned char)pCtx->str[pCtx->pos++];
}

static void _sscanf_ungetc_cb(int c, void* ctx) {
    _sscanf_ctx* pCtx = (_sscanf_ctx*)ctx;
    if (pCtx->pos > 0) pCtx->pos--;
}

int vsscanf(const char* str, const char* format, va_list ap) {
    if (!str) return EOF;
    _sscanf_ctx ctx = { str, 0 };
    return _vcbscanf(_sscanf_getc_cb, _sscanf_ungetc_cb, &ctx, format, ap);
}

int sscanf(const char* str, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vsscanf(str, format, args);
    va_end(args);
    return ret;
}

static int _stdin_getc_cb(void* ctx) {
    (void)ctx;
    return getchar();
}

static void _stdin_ungetc_cb(int c, void* ctx) {
    (void)ctx;
    // Baremetal stdin doesn't formally support ungetc yet, 
    // but for simple scanfs this is rarely a strict blocker unless buffering.
}

int vscanf(const char* format, va_list ap) {
    return _vcbscanf(_stdin_getc_cb, _stdin_ungetc_cb, nullptr, format, ap);
}

int scanf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vscanf(format, args);
    va_end(args);
    return ret;
}

struct _fscanf_ctx {
    FILE* stream;
    int ungot_char;
    bool has_ungot;
};

static int _fscanf_getc_cb(void* ctx) {
    _fscanf_ctx* pCtx = (_fscanf_ctx*)ctx;
    if (pCtx->has_ungot) {
        pCtx->has_ungot = false;
        return pCtx->ungot_char;
    }
    return fgetc(pCtx->stream);
}

static void _fscanf_ungetc_cb(int c, void* ctx) {
    _fscanf_ctx* pCtx = (_fscanf_ctx*)ctx;
    pCtx->ungot_char = c;
    pCtx->has_ungot = true;
}

int vfscanf(FILE* stream, const char* format, va_list ap) {
    if (!stream) return EOF;
    _fscanf_ctx ctx = { stream, 0, false };
    return _vcbscanf(_fscanf_getc_cb, _fscanf_ungetc_cb, &ctx, format, ap);
}

int fscanf(FILE* stream, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vfscanf(stream, format, args);
    va_end(args);
    return ret;
}

int getchar(void) {
    long ret = syscall0(SYS_GETCHAR);
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

    long fd = syscall2(SYS_FOPEN, (long)filename, (long)mode_flag);
    if (fd == -1) return nullptr;

    FILE* f = (FILE*)malloc(sizeof(FILE));
    if (!f) {
        syscall1(SYS_FCLOSE, fd);
        return nullptr;
    }

    f->buffer = (char*)malloc(BUFSIZ);
    if (!f->buffer) {
        syscall1(SYS_FCLOSE, fd);
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
        long bytes = syscall3(SYS_FWRITE, (long)stream->fd, (long)stream->buffer, (long)stream->buffer_pos);
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
    long closed = syscall1(SYS_FCLOSE, (long)stream->fd);
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
        long bytes = syscall3(SYS_FREAD, (long)stream->fd, (long)stream->buffer, (long)stream->buffer_size);
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
            long bytes = syscall3(SYS_FREAD, (long)stream->fd, (long)stream->buffer, (long)stream->buffer_size);
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

int fseek(FILE* stream, long offset, int whence) {
    if (!stream) return -1;
    
    flockfile(stream);
    
    if (stream->mode == FMODE_WRITE) {
        _fflush_unlocked(stream);
    } else if (stream->mode == FMODE_READ) {
        // Discard read buffer
        long actual_offset = syscall3(SYS_FSEEK, (long)stream->fd, 0, SEEK_CUR);
        if (actual_offset != -1) {
             long rewind_amount = stream->bytes_in_buf - stream->buffer_pos;
             if (rewind_amount > 0) {
                 syscall3(SYS_FSEEK, (long)stream->fd, -rewind_amount, SEEK_CUR);
             }
        }
        stream->buffer_pos = 0;
        stream->bytes_in_buf = 0;
        stream->eof = 0;
    }

    long ret = syscall3(SYS_FSEEK, (long)stream->fd, offset, whence);
    funlockfile(stream);
    
    return (ret == -1) ? -1 : 0;
}

long ftell(FILE* stream) {
    if (!stream) return -1;
    
    flockfile(stream);
    long pos = syscall3(SYS_FSEEK, (long)stream->fd, 0, SEEK_CUR);
    if (pos != -1) {
        if (stream->mode == FMODE_WRITE) {
            pos += stream->buffer_pos;
        } else if (stream->mode == FMODE_READ) {
            pos -= (stream->bytes_in_buf - stream->buffer_pos);
        }
    }
    funlockfile(stream);
    
    return pos;
}

void rewind(FILE* stream) {
    fseek(stream, 0, SEEK_SET);
}
