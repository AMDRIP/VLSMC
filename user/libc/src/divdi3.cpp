#include <stdint.h>

extern "C" {

uint64_t __udivdi3(uint64_t num, uint64_t den) {
    if (den == 0) return 0;
    uint64_t quot = 0;
    uint64_t bit = 1;
    while (den <= num && !(den & (1ULL << 63))) {
        den <<= 1;
        bit <<= 1;
    }
    while (bit) {
        if (num >= den) {
            num -= den;
            quot |= bit;
        }
        den >>= 1;
        bit >>= 1;
    }
    return quot;
}

int64_t __divdi3(int64_t a, int64_t b) {
    int neg = 0;
    if (a < 0) { a = -a; neg ^= 1; }
    if (b < 0) { b = -b; neg ^= 1; }
    uint64_t r = __udivdi3((uint64_t)a, (uint64_t)b);
    return neg ? -(int64_t)r : (int64_t)r;
}

uint64_t __umoddi3(uint64_t num, uint64_t den) {
    return num - __udivdi3(num, den) * den;
}

int64_t __moddi3(int64_t a, int64_t b) {
    int neg = 0;
    if (a < 0) { a = -a; neg = 1; }
    if (b < 0) { b = -b; }
    uint64_t r = __umoddi3((uint64_t)a, (uint64_t)b);
    return neg ? -(int64_t)r : (int64_t)r;
}

}
