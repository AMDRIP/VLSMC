#include <ctype.h>

extern "C" {

const unsigned char _ctype_[257] = {
    0, // EOF entry (-1)

    // 0x00 - 0x1F (Control chars)
    _CTYPE_C, _CTYPE_C, _CTYPE_C, _CTYPE_C, _CTYPE_C, _CTYPE_C, _CTYPE_C, _CTYPE_C,
    _CTYPE_C, _CTYPE_C|_CTYPE_S|_CTYPE_B, _CTYPE_C|_CTYPE_S, _CTYPE_C|_CTYPE_S, 
    _CTYPE_C|_CTYPE_S, _CTYPE_C|_CTYPE_S, _CTYPE_C, _CTYPE_C,
    _CTYPE_C, _CTYPE_C, _CTYPE_C, _CTYPE_C, _CTYPE_C, _CTYPE_C, _CTYPE_C, _CTYPE_C,
    _CTYPE_C, _CTYPE_C, _CTYPE_C, _CTYPE_C, _CTYPE_C, _CTYPE_C, _CTYPE_C, _CTYPE_C,

    // 0x20 - 0x2F (Space, Punctuation)
    _CTYPE_S|_CTYPE_B, _CTYPE_P, _CTYPE_P, _CTYPE_P, _CTYPE_P, _CTYPE_P, _CTYPE_P, _CTYPE_P,
    _CTYPE_P, _CTYPE_P, _CTYPE_P, _CTYPE_P, _CTYPE_P, _CTYPE_P, _CTYPE_P, _CTYPE_P,

    // 0x30 - 0x39 (Digits)
    _CTYPE_D, _CTYPE_D, _CTYPE_D, _CTYPE_D, _CTYPE_D, _CTYPE_D, _CTYPE_D, _CTYPE_D,
    _CTYPE_D, _CTYPE_D,

    // 0x3A - 0x40 (Punctuation)
    _CTYPE_P, _CTYPE_P, _CTYPE_P, _CTYPE_P, _CTYPE_P, _CTYPE_P, _CTYPE_P,

    // 0x41 - 0x46 (Upper Hex chars A-F)
    _CTYPE_U|_CTYPE_X, _CTYPE_U|_CTYPE_X, _CTYPE_U|_CTYPE_X, _CTYPE_U|_CTYPE_X,
    _CTYPE_U|_CTYPE_X, _CTYPE_U|_CTYPE_X,

    // 0x47 - 0x5A (Other Upper chars)
    _CTYPE_U, _CTYPE_U, _CTYPE_U, _CTYPE_U, _CTYPE_U, _CTYPE_U, _CTYPE_U, _CTYPE_U,
    _CTYPE_U, _CTYPE_U, _CTYPE_U, _CTYPE_U, _CTYPE_U, _CTYPE_U, _CTYPE_U, _CTYPE_U,
    _CTYPE_U, _CTYPE_U, _CTYPE_U, _CTYPE_U,

    // 0x5B - 0x60 (Punctuation)
    _CTYPE_P, _CTYPE_P, _CTYPE_P, _CTYPE_P, _CTYPE_P, _CTYPE_P,

    // 0x61 - 0x66 (Lower Hex chars a-f)
    _CTYPE_L|_CTYPE_X, _CTYPE_L|_CTYPE_X, _CTYPE_L|_CTYPE_X, _CTYPE_L|_CTYPE_X,
    _CTYPE_L|_CTYPE_X, _CTYPE_L|_CTYPE_X,

    // 0x67 - 0x7A (Other Lower chars)
    _CTYPE_L, _CTYPE_L, _CTYPE_L, _CTYPE_L, _CTYPE_L, _CTYPE_L, _CTYPE_L, _CTYPE_L,
    _CTYPE_L, _CTYPE_L, _CTYPE_L, _CTYPE_L, _CTYPE_L, _CTYPE_L, _CTYPE_L, _CTYPE_L,
    _CTYPE_L, _CTYPE_L, _CTYPE_L, _CTYPE_L,

    // 0x7B - 0x7E (Punctuation)
    _CTYPE_P, _CTYPE_P, _CTYPE_P, _CTYPE_P,

    // 0x7F (Control - Delete)
    _CTYPE_C,

    // 0x80 - 0xFF (Extended ASCII, generally no standard flags applied here unless locale supports it)
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

} // extern "C"
