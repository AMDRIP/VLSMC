#pragma once

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef uint32_t size_t;

#define SYS_EXIT        0
#define SYS_PRINT       1
#define SYS_GETCHAR     2
#define SYS_SLEEP       3
#define SYS_YIELD       4
#define SYS_GETPID      5
#define SYS_MMAP        12
#define SYS_MUNMAP      13
#define SYS_SEND        14
#define SYS_RECV        15
#define SYS_TIME        16
#define SYS_INB         17
#define SYS_OUTB        18
#define SYS_INW         19
#define SYS_OUTW        20
#define SYS_WAIT_IRQ    22
#define SYS_SEND_MSG    23
#define SYS_RECV_MSG    24
#define SYS_READ_SECTOR 25
#define SYS_MAP_MMIO    26
#define SYS_FIND_THREAD 27
#define SYS_SBRK        28
#define SYS_FOPEN       29
#define SYS_FREAD       30
#define SYS_FWRITE      31
#define SYS_FCLOSE      32
#define SYS_FSIZE       33
#define SYS_FORK        34
#define SYS_EXEC        35
#define SYS_WAIT        36
#define SYS_GRANT_MMIO  37
#define SYS_SET_DRIVER  38

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

static inline uint8_t sys_inb(uint16_t port) {
    return (uint8_t)syscall1(SYS_INB, (uint32_t)port);
}

static inline void sys_outb(uint16_t port, uint8_t val) {
    syscall2(SYS_OUTB, (uint32_t)port, (uint32_t)val);
}

static inline uint16_t sys_inw(uint16_t port) {
    return (uint16_t)syscall1(SYS_INW, (uint32_t)port);
}

static inline void sys_outw(uint16_t port, uint16_t val) {
    syscall2(SYS_OUTW, (uint32_t)port, (uint32_t)val);
}

static inline void sys_wait_irq(uint8_t irq) {
    syscall1(SYS_WAIT_IRQ, (uint32_t)irq);
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

static inline int sys_fopen(const char* path, int mode) {
    return (int)syscall2(SYS_FOPEN, (uint32_t)path, (uint32_t)mode);
}

static inline int sys_fread(int fd, void* buf, uint32_t size) {
    return (int)syscall3(SYS_FREAD, (uint32_t)fd, (uint32_t)buf, size);
}

static inline int sys_fwrite(int fd, const void* buf, uint32_t size) {
    return (int)syscall3(SYS_FWRITE, (uint32_t)fd, (uint32_t)buf, size);
}

static inline int sys_fclose(int fd) {
    return (int)syscall1(SYS_FCLOSE, (uint32_t)fd);
}

static inline uint32_t sys_fsize(int fd) {
    return syscall1(SYS_FSIZE, (uint32_t)fd);
}

static inline int sys_fork(void) {
    return (int)syscall0(SYS_FORK);
}

static inline int sys_exec(const char* path, char* const* argv, char* const* envp) {
    return (int)syscall3(SYS_EXEC, (uint32_t)path, (uint32_t)argv, (uint32_t)envp);
}

static inline int sys_wait(int* status) {
    return (int)syscall1(SYS_WAIT, (uint32_t)status);
}

static inline int sys_grant_mmio(int target_tid, uint32_t phys_start, uint32_t phys_end) {
    return (int)syscall3(SYS_GRANT_MMIO, (uint32_t)target_tid, phys_start, phys_end);
}

static inline int sys_set_driver(int target_tid) {
    return (int)syscall1(SYS_SET_DRIVER, (uint32_t)target_tid);
}

static inline void print(const char* s) {
    uint32_t len = 0;
    while (s[len]) len++;
    sys_print(s, len);
}
