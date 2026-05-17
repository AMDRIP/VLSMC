#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define SYS_EXIT        0
#define SYS_PRINT       1
#define SYS_GETCHAR     2
#define SYS_SLEEP       3
#define SYS_YIELD       4
#define SYS_GETPID      5
#define SYS_OPEN        8
#define SYS_READ        9
#define SYS_WRITE       10
#define SYS_CLOSE       11
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
#define SYS_UNMAP_MMIO  39
#define SYS_GET_VGA_INFO 40
#define SYS_UPTIME      41
#define SYS_READDIR     42
#define SYS_FSEEK       43
#define SYS_GRANT_PORT  44
#define SYS_GRANT_IRQ   45
#define SYS_UNLINK      46
#define SYS_STAT        47
#define SYS_FSTAT       48
#define SYS_MKDIR       49
#define SYS_WAITPID     50
#define SYS_LINK        51
#define SYS_SYMLINK     52
#define SYS_READLINK    53
#define SYS_NET_INFO    54
#define SYS_NET_CONFIG  55
#define SYS_NET_SEND_UDP 56
#define SYS_NET_RECV_UDP 57
#define SYS_MPROTECT    58

#ifndef VLSMC_NET_SYSCALL_TYPES
#define VLSMC_NET_SYSCALL_TYPES
struct net_stats {
    uint32_t rx_frames;
    uint32_t tx_frames;
    uint32_t rx_bytes;
    uint32_t tx_bytes;
    uint32_t rx_dropped;
    uint32_t tx_dropped;
    uint32_t arp_rx;
    uint32_t arp_tx;
    uint32_t ipv4_rx;
    uint32_t ipv4_tx;
    uint32_t icmp_rx;
    uint32_t icmp_tx;
    uint32_t udp_rx;
    uint32_t udp_tx;
};

struct net_info {
    bool link_up;
    char driver[16];
    uint8_t mac[6];
    uint32_t ipv4_addr;
    uint32_t netmask;
    uint32_t gateway;
    struct net_stats stats;
};
#endif

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
