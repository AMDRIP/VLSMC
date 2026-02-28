#pragma once

#include <stdint.h>

namespace re36 {

#define SYS_EXIT     0
#define SYS_PRINT    1
#define SYS_GETCHAR  2
#define SYS_SLEEP    3
#define SYS_YIELD    4
#define SYS_GETPID   5
#define SYS_FORK     6
#define SYS_EXEC     7
#define SYS_OPEN     8
#define SYS_READ     9
#define SYS_WRITE    10
#define SYS_CLOSE    11
#define SYS_MMAP     12
#define SYS_MUNMAP   13
#define SYS_SEND     14
#define SYS_RECV     15
#define SYS_TIME     16

#define SYS_GRANT_MMIO 37
#define SYS_SET_DRIVER 38

struct SyscallRegs {
    uint32_t eax; // Номер syscall
    uint32_t ebx; // Аргумент 1
    uint32_t ecx; // Аргумент 2
    uint32_t edx; // Аргумент 3
    uint32_t esi; // Аргумент 4
    uint32_t edi; // Аргумент 5
};

void syscall_gate_init();

uint32_t handle_syscall(SyscallRegs* regs);

} // namespace re36

#include "kernel/idt.h"

namespace re36 {
extern Registers* g_current_isr_regs;
} // namespace re36
