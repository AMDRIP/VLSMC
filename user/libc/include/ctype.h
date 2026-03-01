#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define _CTYPE_U 0x01 // Upper case
#define _CTYPE_L 0x02 // Lower case
#define _CTYPE_D 0x04 // Digit
#define _CTYPE_S 0x08 // Space/Whitespace
#define _CTYPE_P 0x10 // Punctuation
#define _CTYPE_C 0x20 // Control
#define _CTYPE_X 0x40 // Hex digit
#define _CTYPE_B 0x80 // Blank

extern const unsigned char _ctype_[257];

#define isalnum(c)  ((_ctype_)[(unsigned char)(c) + 1] & (_CTYPE_U|_CTYPE_L|_CTYPE_D))
#define isalpha(c)  ((_ctype_)[(unsigned char)(c) + 1] & (_CTYPE_U|_CTYPE_L))
#define iscntrl(c)  ((_ctype_)[(unsigned char)(c) + 1] & (_CTYPE_C))
#define isdigit(c)  ((_ctype_)[(unsigned char)(c) + 1] & (_CTYPE_D))
#define isgraph(c)  ((_ctype_)[(unsigned char)(c) + 1] & (_CTYPE_P|_CTYPE_U|_CTYPE_L|_CTYPE_D))
#define islower(c)  ((_ctype_)[(unsigned char)(c) + 1] & (_CTYPE_L))
#define isprint(c)  ((_ctype_)[(unsigned char)(c) + 1] & (_CTYPE_P|_CTYPE_U|_CTYPE_L|_CTYPE_D|_CTYPE_B))
#define ispunct(c)  ((_ctype_)[(unsigned char)(c) + 1] & (_CTYPE_P))
#define isspace(c)  ((_ctype_)[(unsigned char)(c) + 1] & (_CTYPE_S))
#define isupper(c)  ((_ctype_)[(unsigned char)(c) + 1] & (_CTYPE_U))
#define isxdigit(c) ((_ctype_)[(unsigned char)(c) + 1] & (_CTYPE_D|_CTYPE_X))
#define isblank(c)  ((_ctype_)[(unsigned char)(c) + 1] & (_CTYPE_B))
#define isascii(c)  (((unsigned char)(c)) <= 0x7F)

static inline int tolower(int c) {
    if (isupper(c)) return c + 32;
    return c;
}

static inline int toupper(int c) {
    if (islower(c)) return c - 32;
    return c;
}

#ifdef __cplusplus
}
#endif
