#include "sys/syscall.h"
#include "errno.h"
#include <stdarg.h>

extern "C" long syscall(long number, ...) {
    va_list args;
    va_start(args, number);

    long a1 = va_arg(args, long);
    long a2 = va_arg(args, long);
    long a3 = va_arg(args, long);
    long a4 = va_arg(args, long);
    long a5 = va_arg(args, long);

    va_end(args);

    long ret;
    asm volatile(
        "int $0x80"
        : "=a" (ret)
        : "a" (number), "b" (a1), "c" (a2), "d" (a3), "S" (a4), "D" (a5)
        : "memory"
    );

    if (ret < 0 && ret > -4096) {
        errno = -ret;
        return -1;
    }

    return ret;
}
