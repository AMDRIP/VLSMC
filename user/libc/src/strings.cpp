#include <strings.h>
#include <ctype.h>

int strcasecmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        int d = tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
        if (d != 0) return d;
        s1++; s2++;
    }
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

int strncasecmp(const char *s1, const char *s2, size_t n) {
    if (n == 0) return 0;
    while (*s1 && *s2 && n > 1) {
        int d = tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
        if (d != 0) return d;
        s1++; s2++; n--;
    }
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

void bzero(void *s, size_t n) {
    char *p = (char *)s;
    while (n--) *p++ = 0;
}

void bcopy(const void *src, void *dest, size_t n) {
    const char *s = (const char *)src;
    char *d = (char *)dest;
    while (n--) *d++ = *s++;
}

char *index(const char *s, int c) {
    while (*s != (char)c) {
        if (!*s) return nullptr;
        s++;
    }
    return (char *)s;
}

char *rindex(const char *s, int c) {
    const char *last = nullptr;
    do {
        if (*s == (char)c) last = s;
    } while (*s++);
    return (char *)last;
}

int ffs(int i) {
    if (i == 0) return 0;
    int pos = 1;
    while (!(i & 1)) {
        i >>= 1;
        pos++;
    }
    return pos;
}
