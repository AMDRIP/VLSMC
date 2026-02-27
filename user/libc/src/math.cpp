#include "math.h"

extern "C" double fabs(double x) {
    double res;
    asm volatile ("fabs" : "=t" (res) : "0" (x));
    return res;
}

extern "C" float fabsf(float x) {
    float res;
    asm volatile ("fabs" : "=t" (res) : "0" (x));
    return res;
}

extern "C" double sqrt(double x) {
    double res;
    asm volatile ("fsqrt" : "=t" (res) : "0" (x));
    return res;
}

extern "C" float sqrtf(float x) {
    float res;
    asm volatile ("fsqrt" : "=t" (res) : "0" (x));
    return res;
}

extern "C" double sin(double x) {
    double res;
    asm volatile ("fsin" : "=t" (res) : "0" (x));
    return res;
}

extern "C" float sinf(float x) {
    float res;
    asm volatile ("fsin" : "=t" (res) : "0" (x));
    return res;
}

extern "C" double cos(double x) {
    double res;
    asm volatile ("fcos" : "=t" (res) : "0" (x));
    return res;
}

extern "C" float cosf(float x) {
    float res;
    asm volatile ("fcos" : "=t" (res) : "0" (x));
    return res;
}

extern "C" double pow(double base, double exp) {
    if (base == 0.0) return 0.0;
    if (exp == 0.0) return 1.0;

    double res;
    asm volatile (
        "fyl2x;"
        "fld %%st(0);"
        "frndint;"
        "fxch %%st(1);"
        "fsub %%st(1), %%st(0);"
        "f2xm1;"
        "fld1;"
        "faddp;"
        "fscale;"
        "fstp %%st(1);"
        : "=t" (res)
        : "0" (base), "u" (exp)
    );
    return res;
}

extern "C" float powf(float base, float exp) {
    return (float)pow((double)base, (double)exp);
}
