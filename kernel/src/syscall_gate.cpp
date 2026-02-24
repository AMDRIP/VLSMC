#include "kernel/syscall_gate.h"
#include "kernel/idt.h"
#include "kernel/timer.h"
#include "kernel/task_scheduler.h"
#include "kernel/thread.h"
#include "kernel/event_channel.h"
#include "kernel/vmm.h"
#include "kernel/keyboard.h"
#include "libc.h"

namespace re36 {

extern "C" void isr128();

void syscall_gate_init() {
    set_idt_gate(128, (uint32_t)isr128, 0x08, 0xEE);
    // 0xEE = Present(1) | DPL=3(11) | Gate Type=Interrupt(01110)
    // DPL=3 позволяет вызов из Ring 3
}

static uint32_t sys_exit(SyscallRegs* regs) {
    (void)regs;
    TaskScheduler::terminate_current();
    return 0;
}

static uint32_t sys_print(SyscallRegs* regs) {
    const char* str = (const char*)regs->ebx;
    uint32_t len = regs->ecx;
    
    for (uint32_t i = 0; i < len && str[i] != '\0'; i++) {
        putchar(str[i]);
    }
    return len;
}

static uint32_t sys_getchar(SyscallRegs* regs) {
    (void)regs;
    return (uint32_t)KeyboardDriver::get_char();
}

static uint32_t sys_sleep(SyscallRegs* regs) {
    uint32_t ms = regs->ebx;
    Timer::sleep(ms);
    return 0;
}

static uint32_t sys_yield(SyscallRegs* regs) {
    (void)regs;
    thread_yield();
    return 0;
}

static uint32_t sys_getpid(SyscallRegs* regs) {
    (void)regs;
    return (uint32_t)TaskScheduler::get_current_tid();
}

static uint32_t sys_time(SyscallRegs* regs) {
    (void)regs;
    return Timer::get_ticks();
}

static uint32_t sys_send(SyscallRegs* regs) {
    int channel_id = (int)regs->ebx;
    uint32_t data = regs->ecx;
    return EventSystem::push(channel_id, data) ? 0 : (uint32_t)-1;
}

static uint32_t sys_recv(SyscallRegs* regs) {
    int channel_id = (int)regs->ebx;
    bool blocking = (regs->ecx != 0);
    
    if (blocking) {
        return EventSystem::wait(channel_id);
    }
    return EventSystem::pop(channel_id);
}

static uint32_t sys_mmap(SyscallRegs* regs) {
    uint32_t virt = regs->ebx;
    uint32_t phys = regs->ecx;
    uint32_t flags = regs->edx;
    VMM::map_page(virt, phys, flags);
    return 0;
}

static uint32_t sys_munmap(SyscallRegs* regs) {
    uint32_t virt = regs->ebx;
    VMM::unmap_page(virt);
    return 0;
}

typedef uint32_t (*SyscallHandler)(SyscallRegs*);

static SyscallHandler syscall_table[] = {
    sys_exit,       // 0
    sys_print,      // 1
    sys_getchar,    // 2
    sys_sleep,      // 3
    sys_yield,      // 4
    sys_getpid,     // 5
    nullptr,        // 6 (fork — TODO)
    nullptr,        // 7 (exec — TODO)
    nullptr,        // 8 (open — TODO)
    nullptr,        // 9 (read — TODO)
    nullptr,        // 10 (write — TODO)
    nullptr,        // 11 (close — TODO)
    sys_mmap,       // 12
    sys_munmap,     // 13
    sys_send,       // 14
    sys_recv,       // 15
    sys_time,       // 16
};

#define SYSCALL_COUNT (sizeof(syscall_table) / sizeof(syscall_table[0]))

uint32_t handle_syscall(SyscallRegs* regs) {
    uint32_t syscall_num = regs->eax;
    
    if (syscall_num >= SYSCALL_COUNT || syscall_table[syscall_num] == nullptr) {
        printf("[SYSCALL] Unknown syscall #%d from TID %d\n", syscall_num, current_tid);
        return (uint32_t)-1;
    }

    return syscall_table[syscall_num](regs);
}

} // namespace re36
