#pragma once

#include <stdint.h>
#include <stddef.h>

#define SYS_EXIT     0
#define SYS_PRINT    1
#define SYS_GETCHAR  2
#define SYS_SLEEP    3
#define SYS_YIELD    4
#define SYS_GETPID   5
#define SYS_TIME     16
#define SYS_SEND_MSG 23
#define SYS_RECV_MSG 24
#define SYS_READ_SECTOR 25
#define SYS_MAP_MMIO 26
#define SYS_FIND_THREAD 27
#define SYS_SBRK 28

#ifdef __cplusplus
extern "C" {
#endif

long syscall(long number, ...);

static inline long __syscall0(long n) {
    long ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(n) : "memory");
    return ret;
}

static inline long __syscall1(long n, long a1) {
    long ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(n), "b"(a1) : "memory");
    return ret;
}

static inline long __syscall2(long n, long a1, long a2) {
    long ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(n), "b"(a1), "c"(a2) : "memory");
    return ret;
}

static inline long __syscall3(long n, long a1, long a2, long a3) {
    long ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(n), "b"(a1), "c"(a2), "d"(a3) : "memory");
    return ret;
}

static inline long __syscall4(long n, long a1, long a2, long a3, long a4) {
    long ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(n), "b"(a1), "c"(a2), "d"(a3), "S"(a4) : "memory");
    return ret;
}

static inline long __syscall5(long n, long a1, long a2, long a3, long a4, long a5) {
    long ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(n), "b"(a1), "c"(a2), "d"(a3), "S"(a4), "D"(a5) : "memory");
    return ret;
}

#ifdef __cplusplus
}
#endif
