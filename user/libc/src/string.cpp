#include "string.h"
#include <stdint.h>

extern "C" void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

extern "C" void* memmove(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    if (d < s) {
        while (n--) {
            *d++ = *s++;
        }
    } else {
        d += n;
        s += n;
        while (n--) {
            *(--d) = *(--s);
        }
    }
    return dest;
}

extern "C" void* memset(void* s, int c, size_t n) {
    uint8_t* p = (uint8_t*)s;
    while (n--) {
        *p++ = (uint8_t)c;
    }
    return s;
}

extern "C" int memcmp(const void* s1, const void* s2, size_t n) {
    const uint8_t* p1 = (const uint8_t*)s1;
    const uint8_t* p2 = (const uint8_t*)s2;
    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}

extern "C" size_t strlen(const char* s) {
    if (!s) return 0;
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

extern "C" size_t strnlen(const char* s, size_t maxlen) {
    if (!s) return 0;
    size_t len = 0;
    while (len < maxlen && s[len]) len++;
    return len;
}

extern "C" char* strcpy(char* dest, const char* src) {
    if (!dest || !src) return dest;
    char* d = dest;
    while ((*d++ = *src++)) {}
    return dest;
}

extern "C" size_t strlcpy(char* dest, const char* src, size_t size) {
    if (!dest || !src || size == 0) return 0;
    size_t src_len = strlen(src);
    size_t to_copy = src_len < (size - 1) ? src_len : (size - 1);
    memcpy(dest, src, to_copy);
    dest[to_copy] = '\0';
    return src_len;
}

extern "C" int strcmp(const char* s1, const char* s2) {
    if (!s1 || !s2) return 0;
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

extern "C" int strncmp(const char* s1, const char* s2, size_t n) {
    if (!s1 || !s2 || n == 0) return 0;
    while (n-- > 0) {
        if (*s1 != *s2) {
            return *(const unsigned char*)s1 - *(const unsigned char*)s2;
        }
        if (*s1 == '\0') {
            return 0;
        }
        s1++;
        s2++;
    }
    return 0;
}

extern "C" int strncmp_s(const char* s1, size_t s1max, const char* s2, size_t s2max) {
    if (!s1 || !s2) return 0;
    size_t n = s1max < s2max ? s1max : s2max;
    return strncmp(s1, s2, n);
}

extern "C" char* strchr(const char* s, int c) {
    if (!s) return nullptr;
    while (*s) {
        if (*s == (char)c) return (char*)s;
        s++;
    }
    if (c == '\0') return (char*)s;
    return nullptr;
}

extern "C" char* strstr(const char* haystack, const char* needle) {
    if (!haystack || !needle) return nullptr;
    if (!*needle) return (char*)haystack;
    while (*haystack) {
        const char* h = haystack;
        const char* n = needle;
        while (*h && *n && (*h == *n)) {
            h++;
            n++;
        }
        if (!*n) return (char*)haystack;
        haystack++;
    }
    return nullptr;
}
