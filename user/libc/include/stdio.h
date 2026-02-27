#pragma once

#include <stddef.h>
#include <stdarg.h>

#define EOF (-1)

#ifdef __cplusplus
extern "C" {
#endif

int putchar(int c);
int puts(const char* s);
int printf(const char* format, ...);

int getchar(void);
char* gets_s(char* buffer, size_t size);

typedef struct {
    int fd;
    int mode;
    int eof;
    int error;
} FILE;

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define FMODE_READ 1
#define FMODE_WRITE 2

FILE* fopen(const char* filename, const char* mode);
int fclose(FILE* stream);
size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream);
size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream);
int feof(FILE* stream);
int ferror(FILE* stream);

#ifdef __cplusplus
}
#endif
