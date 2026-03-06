#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t n);
void bzero(void *s, size_t n);
void bcopy(const void *src, void *dest, size_t n);
char *index(const char *s, int c);
char *rindex(const char *s, int c);
int ffs(int i);

#ifdef __cplusplus
}
#endif
