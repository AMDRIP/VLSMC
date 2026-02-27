#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void* memcpy(void* dest, const void* src, size_t n);
void* memmove(void* dest, const void* src, size_t n);
void* memset(void* s, int c, size_t n);
int memcmp(const void* s1, const void* s2, size_t n);

size_t strlen(const char* s);
size_t strnlen(const char* s, size_t maxlen);
char* strcpy(char* dest, const char* src);
size_t strlcpy(char* dest, const char* src, size_t size);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);
int strncmp_s(const char* s1, size_t s1max, const char* s2, size_t s2max);
char* strchr(const char* s, int c);
char* strstr(const char* haystack, const char* needle);

#ifdef __cplusplus
}
#endif
