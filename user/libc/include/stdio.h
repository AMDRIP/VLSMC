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

#ifdef __cplusplus
}
#endif
