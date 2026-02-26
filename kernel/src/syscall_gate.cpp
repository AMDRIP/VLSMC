#include "kernel/syscall_gate.h"
#include "kernel/idt.h"
#include "kernel/timer.h"
#include "kernel/task_scheduler.h"
#include "kernel/thread.h"
#include "kernel/event_channel.h"
#include "kernel/vmm.h"
#include "kernel/keyboard.h"
#include "kernel/pic.h"
#include "kernel/ata.h"
#include "kernel/spinlock.h"
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
    // Оставляем пустую заглушку. Явный yield внутри системного вызова 
    // ломает QEMU, так как CPL=0 не возвращается через iret.
    // Пользовательские процессы должны использовать sys_sleep()
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

static uint32_t sys_inb(SyscallRegs* regs) {
    uint16_t port = (uint16_t)regs->ebx;
    return inb(port);
}

static uint32_t sys_outb(SyscallRegs* regs) {
    uint16_t port = (uint16_t)regs->ebx;
    uint8_t data = (uint8_t)regs->ecx;
    outb(port, data);
    return 0;
}

static uint32_t sys_inw(SyscallRegs* regs) {
    uint16_t port = (uint16_t)regs->ebx;
    return inw(port);
}

static uint32_t sys_outw(SyscallRegs* regs) {
    uint16_t port = (uint16_t)regs->ebx;
    uint16_t data = (uint16_t)regs->ecx;
    outw(port, data);
    return 0;
}

static uint32_t sys_map_mmio(SyscallRegs* regs) {
    uint32_t virt = regs->ebx;
    uint32_t phys = regs->ecx;
    uint32_t size_pages = regs->edx;
    
    printf("[SYSCALL] map_mmio: virt=0x%x, phys=0x%x, pages=%d\n", virt, phys, size_pages);

    // Простейшая проверка безопасности: не даем маппить в адресное пространство ядра
    if (virt < KERNEL_SPACE_END) {
        printf("[SYSCALL] map_mmio failed: virt addr 0x%x is below KERNEL_SPACE_END\n", virt);
        return 0; // Ошибка
    }

    // Маппим каждую страницу
    for (uint32_t i = 0; i < size_pages; i++) {
        uint32_t v = virt + i * 4096;
        uint32_t p = phys + i * 4096;
        VMM::map_page(v, p, PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);
    }
    
    return virt;
}

static uint32_t sys_wait_irq(SyscallRegs* regs) {
    uint8_t irq = (uint8_t)regs->ebx;
    EventSystem::wait((int)irq + 1000);
    return 0;
}

static uint32_t sys_send_msg(SyscallRegs* regs) {
    int target_tid = (int)regs->ebx;
    const uint8_t* data = (const uint8_t*)regs->ecx;
    uint32_t size = regs->edx;

    if (target_tid < 0 || target_tid >= MAX_THREADS || size > IPC_MAX_MSG_SIZE) return (uint32_t)-1;
    
    InterruptGuard guard;
    Thread& target = threads[target_tid];
    if (target.state == ThreadState::Unused || target.state == ThreadState::Terminated) return (uint32_t)-1;

    if (target.msg_count >= IPC_MSG_QUEUE_SIZE) return (uint32_t)-1; // Queue full

    IpcMessage& msg = target.messages[target.msg_tail];
    msg.sender_tid = current_tid;
    msg.size = size;
    for (uint32_t i = 0; i < size; i++) {
        msg.data[i] = data[i];
    }

    target.msg_tail = (target.msg_tail + 1) % IPC_MSG_QUEUE_SIZE;
    target.msg_count++;

    if (target.waiting_for_msg) {
        target.waiting_for_msg = false;
        TaskScheduler::unblock(target_tid);
    }

    return 0;
}

static uint32_t sys_recv_msg(SyscallRegs* regs) {
    int* sender_tid_out = (int*)regs->ebx;
    uint8_t* buffer = (uint8_t*)regs->ecx;
    uint32_t max_size = regs->edx;

    while (true) {
        {
            InterruptGuard guard;
            Thread& cur = threads[current_tid];

            if (cur.msg_count > 0) {
                IpcMessage& msg = cur.messages[cur.msg_head];
                if (sender_tid_out) *sender_tid_out = msg.sender_tid;

                uint32_t copy_sz = msg.size < max_size ? msg.size : max_size;
                for (uint32_t i = 0; i < copy_sz; i++) {
                    buffer[i] = msg.data[i];
                }

                cur.msg_head = (cur.msg_head + 1) % IPC_MSG_QUEUE_SIZE;
                cur.msg_count--;

                return copy_sz;
            }

            cur.waiting_for_msg = true;
        }

        TaskScheduler::block_current(-1);
    }
}

static uint32_t sys_read_sector(SyscallRegs* regs) {
    uint32_t lba = regs->ebx;
    uint8_t* buffer = (uint8_t*)regs->ecx;
    return ATA::read_sectors(lba, 1, buffer) ? 1 : 0;
}

typedef uint32_t (*SyscallHandler)(SyscallRegs*);

static SyscallHandler syscall_table[] = {
    sys_exit,        // 0
    sys_print,       // 1
    sys_getchar,     // 2
    sys_sleep,       // 3
    sys_yield,       // 4
    sys_getpid,      // 5
    nullptr,         // 6
    nullptr,         // 7
    nullptr,         // 8
    nullptr,         // 9
    nullptr,         // 10
    nullptr,         // 11
    sys_mmap,        // 12
    sys_munmap,      // 13
    sys_send,        // 14
    sys_recv,        // 15
    sys_time,        // 16
    sys_inb,         // 17
    sys_outb,        // 18
    sys_inw,         // 19
    sys_outw,        // 20
    nullptr,         // 21
    sys_wait_irq,    // 22
    sys_send_msg,    // 23
    sys_recv_msg,    // 24
    sys_read_sector, // 25
    sys_map_mmio,    // 26
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
