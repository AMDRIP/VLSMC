#pragma once

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef uint32_t size_t;

static inline uint32_t syscall0(uint32_t num) {
    uint32_t ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(num));
    return ret;
}

static inline uint32_t syscall1(uint32_t num, uint32_t arg1) {
    uint32_t ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(arg1));
    return ret;
}

static inline uint32_t syscall2(uint32_t num, uint32_t arg1, uint32_t arg2) {
    uint32_t ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(arg1), "c"(arg2));
    return ret;
}

static inline uint32_t syscall3(uint32_t num, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    uint32_t ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(arg1), "c"(arg2), "d"(arg3));
    return ret;
}

#define SYS_EXIT     0
#define SYS_PRINT    1
#define SYS_GETCHAR  2
#define SYS_SLEEP    3
#define SYS_YIELD    4
#define SYS_GETPID   5
#define SYS_TIME     16

static inline void sys_exit(void) {
    syscall0(SYS_EXIT);
}

static inline void sys_print(const char* str, uint32_t len) {
    syscall2(SYS_PRINT, (uint32_t)str, len);
}

static inline char sys_getchar(void) {
    return (char)syscall0(SYS_GETCHAR);
}

static inline void sys_sleep(uint32_t ms) {
    syscall1(SYS_SLEEP, ms);
}

static inline void sys_yield(void) {
    syscall0(SYS_YIELD);
}

static inline uint32_t sys_getpid(void) {
    return syscall0(SYS_GETPID);
}

static inline uint32_t sys_time(void) {
    return syscall0(SYS_TIME);
}

static inline void print(const char* s) {
    uint32_t len = 0;
    while (s[len]) len++;
    sys_print(s, len);
}
