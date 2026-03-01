#pragma once

#include <stddef.h>
#include <stdarg.h>

#define EOF (-1)

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration if mutex.h hasn't been fully included at root level.
// Since sys/mutex.h is internal, we use a typedef for FILE's lock.
typedef volatile int _mutex_t;

int putchar(int c);
int puts(const char* s);

int printf(const char* format, ...);
int sprintf(char* str, const char* format, ...);
int snprintf(char* str, size_t size, const char* format, ...);

int vprintf(const char* format, va_list ap);
int vsprintf(char* str, const char* format, va_list ap);
int vsnprintf(char* str, size_t size, const char* format, va_list ap);

int scanf(const char* format, ...);
int sscanf(const char* str, const char* format, ...);

int vscanf(const char* format, va_list ap);
int vsscanf(const char* str, const char* format, va_list ap);

int getchar(void);
char* gets_s(char* buffer, size_t size);

#define BUFSIZ 4096

typedef struct {
    int fd;
    int mode;
    int eof;
    int error;
    char* buffer;
    size_t buffer_size;
    size_t buffer_pos;
    size_t bytes_in_buf;
    _mutex_t lock;
} FILE;

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define FMODE_READ 1
#define FMODE_WRITE 2

int fprintf(FILE* stream, const char* format, ...);
int vfprintf(FILE* stream, const char* format, va_list ap);

int fscanf(FILE* stream, const char* format, ...);
int vfscanf(FILE* stream, const char* format, va_list ap);

FILE* fopen(const char* filename, const char* mode);
int fclose(FILE* stream);
int fflush(FILE* stream);
size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream);
size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream);
int fgetc(FILE* stream);
int fputc(int c, FILE* stream);
int feof(FILE* stream);
int ferror(FILE* stream);

int fseek(FILE* stream, long offset, int whence);
long ftell(FILE* stream);
void rewind(FILE* stream);
void flockfile(FILE* stream);
void funlockfile(FILE* stream);

#ifdef __cplusplus
}
#endif
