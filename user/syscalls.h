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

#define SYS_SEND_MSG 23
#define SYS_RECV_MSG 24
#define SYS_READ_SECTOR 25
#define SYS_MAP_MMIO 26
#define SYS_FIND_THREAD 27
#define SYS_SBRK 28

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

static inline int sys_send_msg(int target_tid, const void* buffer, uint32_t size) {
    return (int)syscall3(SYS_SEND_MSG, (uint32_t)target_tid, (uint32_t)buffer, size);
}

static inline int sys_recv_msg(int* sender_tid_out, void* buffer, uint32_t max_size) {
    return (int)syscall3(SYS_RECV_MSG, (uint32_t)sender_tid_out, (uint32_t)buffer, max_size);
}

static inline bool sys_read_sector(uint32_t lba, void* buffer) {
    return syscall2(SYS_READ_SECTOR, lba, (uint32_t)buffer) != 0;
}

static inline void* sys_map_mmio(uint32_t virt_addr, uint32_t phys_addr, uint32_t size_pages) {
    return (void*)syscall3(SYS_MAP_MMIO, virt_addr, phys_addr, size_pages);
}

static inline int sys_find_thread(const char* name) {
    return (int)syscall1(SYS_FIND_THREAD, (uint32_t)name);
}

static inline void* sys_sbrk(int increment) {
    return (void*)syscall1(SYS_SBRK, (uint32_t)increment);
}

static inline void print(const char* s) {
    uint32_t len = 0;
    while (s[len]) len++;
    sys_print(s, len);
}
