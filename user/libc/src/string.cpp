#include "string.h"
#include <stdint.h>

#define SWAR_MASK_01 0x01010101
#define SWAR_MASK_80 0x80808080
#define HAS_ZERO_BYTE(v) (((v) - SWAR_MASK_01) & ~(v) & SWAR_MASK_80)
#define ALIGN_4(p) (((uint32_t)(p) & 3) == 0)

extern "C" void* memcpy(void* dest, const void* src, size_t n) {
    if (!dest || !src || n == 0) return dest;

    uint8_t* d8 = (uint8_t*)dest;
    const uint8_t* s8 = (const uint8_t*)src;

    if (ALIGN_4(d8) && ALIGN_4(s8)) {
        uint32_t* d32 = (uint32_t*)d8;
        const uint32_t* s32 = (const uint32_t*)s8;
        size_t n32 = n / 4;
        while (n32--) {
            *d32++ = *s32++;
        }
        d8 = (uint8_t*)d32;
        s8 = (const uint8_t*)s32;
        n &= 3;
    }

    while (n--) {
        *d8++ = *s8++;
    }

    return dest;
}

extern "C" void* memmove(void* dest, const void* src, size_t n) {
    if (!dest || !src || n == 0 || dest == src) return dest;

    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;

    if (d < s) {
        return memcpy(dest, src, n);
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
    if (!s || n == 0) return s;

    uint8_t* p = (uint8_t*)s;
    uint8_t val8 = (uint8_t)c;
    
    while (!ALIGN_4(p) && n > 0) {
        *p++ = val8;
        n--;
    }

    if (n >= 4) {
        uint32_t val32 = val8 | (val8 << 8) | (val8 << 16) | (val8 << 24);
        uint32_t* p32 = (uint32_t*)p;
        size_t n32 = n / 4;
        while (n32--) {
            *p32++ = val32;
        }
        p = (uint8_t*)p32;
        n &= 3;
    }

    while (n--) {
        *p++ = val8;
    }

    return s;
}

extern "C" int memcmp(const void* s1, const void* s2, size_t n) {
    if (n == 0) return 0;
    const uint8_t* p1 = (const uint8_t*)s1;
    const uint8_t* p2 = (const uint8_t*)s2;

    if (ALIGN_4(p1) && ALIGN_4(p2)) {
        const uint32_t* p32_1 = (const uint32_t*)p1;
        const uint32_t* p32_2 = (const uint32_t*)p2;
        size_t n32 = n / 4;
        while (n32--) {
            if (*p32_1 != *p32_2) break;
            p32_1++;
            p32_2++;
        }
        p1 = (const uint8_t*)p32_1;
        p2 = (const uint8_t*)p32_2;
        n -= ((const uint8_t*)p32_1 - (const uint8_t*)s1);
    }

    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}

extern "C" int memcmp_s(const void* s1, size_t s1max, const void* s2, size_t s2max) {
    if (!s1 || !s2) return 0;
    size_t n = s1max < s2max ? s1max : s2max;
    return memcmp(s1, s2, n);
}

extern "C" size_t strlen(const char* s) {
    if (!s) return 0;
    const char* p = s;

    while (!ALIGN_4(p)) {
        if (*p == 0) return p - s;
        p++;
    }

    const uint32_t* p32 = (const uint32_t*)p;
    while (true) {
        uint32_t v = *p32;
        if (HAS_ZERO_BYTE(v)) {
            const char* cp = (const char*)p32;
            if (cp[0] == 0) return cp - s;
            if (cp[1] == 0) return cp + 1 - s;
            if (cp[2] == 0) return cp + 2 - s;
            if (cp[3] == 0) return cp + 3 - s;
        }
        p32++;
    }
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
    const char* s = src;

    if (ALIGN_4(d) && ALIGN_4(s)) {
        uint32_t* d32 = (uint32_t*)d;
        const uint32_t* s32 = (const uint32_t*)s;
        while (true) {
            uint32_t v = *s32;
            if (HAS_ZERO_BYTE(v)) {
                break;
            }
            *d32++ = v;
            s32++;
        }
        d = (char*)d32;
        s = (const char*)s32;
    }

    while ((*d++ = *s++)) {}
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

extern "C" char* strcat(char* dest, const char* src) {
    if (!dest || !src) return dest;
    char* d = dest + strlen(dest);
    strcpy(d, src);
    return dest;
}

extern "C" char* strncat(char* dest, const char* src, size_t n) {
    if (!dest || !src || n == 0) return dest;
    char* d = dest + strlen(dest);
    while (n-- > 0 && *src) {
        *d++ = *src++;
    }
    *d = '\0';
    return dest;
}

extern "C" size_t strlcat(char* dest, const char* src, size_t size) {
    if (!dest || !src || size == 0) return 0;
    size_t dest_len = strlen(dest);
    size_t src_len = strlen(src);
    if (dest_len >= size) return size + src_len;

    size_t to_copy = size - dest_len - 1;
    if (to_copy > src_len) to_copy = src_len;

    memcpy(dest + dest_len, src, to_copy);
    dest[dest_len + to_copy] = '\0';
    
    return dest_len + src_len;
}

extern "C" int strcmp(const char* s1, const char* s2) {
    if (!s1 || !s2) return 0;

    if (ALIGN_4(s1) && ALIGN_4(s2)) {
        const uint32_t* p32_1 = (const uint32_t*)s1;
        const uint32_t* p32_2 = (const uint32_t*)s2;
        while (true) {
            uint32_t v1 = *p32_1;
            uint32_t v2 = *p32_2;
            if (v1 != v2 || HAS_ZERO_BYTE(v1)) {
                break;
            }
            p32_1++;
            p32_2++;
        }
        s1 = (const char*)p32_1;
        s2 = (const char*)p32_2;
    }

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

extern "C" char* strrchr(const char* s, int c) {
    if (!s) return nullptr;
    const char* last = nullptr;
    while (*s) {
        if (*s == (char)c) last = s;
        s++;
    }
    if (c == '\0') return (char*)s;
    return (char*)last;
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
