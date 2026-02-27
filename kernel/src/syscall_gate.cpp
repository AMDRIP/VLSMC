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
#include "kernel/pmm.h"
#include "kernel/fat16.h"
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

static uint32_t sys_find_thread(SyscallRegs* regs) {
    const char* target_name = (const char*)regs->ebx;
    if (!target_name) return (uint32_t)-1;

    InterruptGuard guard;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (threads[i].state != ThreadState::Unused && threads[i].state != ThreadState::Terminated) {
            // Сравнение строк
            bool match = true;
            for (int j = 0; j < 32; j++) {
                if (threads[i].name[j] != target_name[j]) {
                    match = false;
                    break;
                }
                if (threads[i].name[j] == '\0' && target_name[j] == '\0') {
                    break; // Конец обеих строк
                }
            }
            if (match) {
                return (uint32_t)i;
            }
        }
    }
    return (uint32_t)-1;
}

static uint32_t sys_sbrk(SyscallRegs* regs) {
    int increment = (int)regs->ebx;
    Thread& cur = threads[current_tid];
    
    // Блокируем кучу (spinlock)
    while (__atomic_test_and_set(&cur.heap_lock, __ATOMIC_ACQUIRE)) {
        asm volatile("pause");
    }
    
    if (increment == 0) {
        uint32_t ret = cur.heap_end;
        __atomic_clear(&cur.heap_lock, __ATOMIC_RELEASE);
        return ret;
    }

    uint32_t old_end = cur.heap_end;
    uint32_t new_end = old_end + increment;

    // Overflow проверка
    if (increment > 0 && new_end < old_end) {
        __atomic_clear(&cur.heap_lock, __ATOMIC_RELEASE);
        return (uint32_t)-1;
    }
    if (increment < 0 && new_end > old_end) {
        __atomic_clear(&cur.heap_lock, __ATOMIC_RELEASE);
        return (uint32_t)-1;
    }

    if (new_end < cur.heap_start) {
        __atomic_clear(&cur.heap_lock, __ATOMIC_RELEASE);
        return (uint32_t)-1;
    }
    
    // Куча не должна лезть в область ядра
    if (new_end < KERNEL_SPACE_END) {
        __atomic_clear(&cur.heap_lock, __ATOMIC_RELEASE);
        return (uint32_t)-1;
    }
    
    // Лимит user space (не задеваем стек, который на 0xB0000000 - 0xB0010000)
    // Лимит кучи ставим 0x8FFFFFFF.
    if (new_end >= 0x8FFFFFFF) {
        __atomic_clear(&cur.heap_lock, __ATOMIC_RELEASE);
        return (uint32_t)-1;
    }
    
    if (increment < 0) {
        // Мы сужаем кучу. Вычисляем страницы, которые больше не нужны
        uint32_t old_page_end = (old_end + 0xFFF) & ~0xFFF;
        uint32_t new_page_end = (new_end + 0xFFF) & ~0xFFF;

        for (uint32_t p = new_page_end; p < old_page_end; p += 4096) {
            uint32_t phys = VMM::get_physical(p);
            if (phys) {
                PhysicalMemoryManager::free_frame((void*)phys);
                VMM::unmap_page(p);
            }
        }
    }
    
    cur.heap_end = new_end;
    __atomic_clear(&cur.heap_lock, __ATOMIC_RELEASE);
    return old_end;
}

static uint32_t sys_read_sector(SyscallRegs* regs) {
    uint32_t lba = regs->ebx;
    uint8_t* buffer = (uint8_t*)regs->ecx;
    return ATA::read_sectors(lba, 1, buffer) ? 1 : 0;
}

#define MAX_OPEN_FILES 16
#define FMODE_READ 1
#define FMODE_WRITE 2

struct KernelFileDesc {
    bool in_use;
    char name[12];
    uint32_t offset;
    uint32_t size;
    int mode;
    uint8_t* write_buf;
    uint32_t write_len;
    uint32_t write_cap;
};

static KernelFileDesc open_files[MAX_OPEN_FILES];

static int find_free_fd() {
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (!open_files[i].in_use) return i;
    }
    return -1;
}

static uint32_t sys_fopen(SyscallRegs* regs) {
    const char* name = (const char*)regs->ebx;
    int mode = (int)regs->ecx;
    if (!name) return (uint32_t)-1;

    int fd = find_free_fd();
    if (fd < 0) return (uint32_t)-1;

    KernelFileDesc& f = open_files[fd];

    for (int i = 0; i < 11; i++) f.name[i] = ' ';
    f.name[11] = '\0';
    int ni = 0;
    int ei = 0;
    bool in_ext = false;
    for (int i = 0; name[i] && i < 12; i++) {
        if (name[i] == '.') { in_ext = true; continue; }
        char c = name[i];
        if (c >= 'a' && c <= 'z') c -= 32;
        if (!in_ext && ni < 8) f.name[ni++] = c;
        else if (in_ext && ei < 3) f.name[8 + ei++] = c;
    }

    if (mode == FMODE_READ) {
        uint8_t tmp_buf[1];
        int result = Fat16::read_file_offset(name, 0, tmp_buf, 0);
        (void)result;

        uint32_t sector_out;
        int index_out;
        int found = Fat16::find_dir_entry(name, &sector_out, &index_out);
        if (found < 0) return (uint32_t)-1;

        uint8_t dir_buf[512];
        ATA::read_sectors(sector_out, 1, dir_buf);
        FAT16_DirEntry* entries = (FAT16_DirEntry*)dir_buf;
        f.size = entries[index_out].file_size;
    } else {
        f.size = 0;
    }

    f.in_use = true;
    f.offset = 0;
    f.mode = mode;
    f.write_buf = nullptr;
    f.write_len = 0;
    f.write_cap = 0;

    return (uint32_t)(fd + 3);
}

static uint32_t sys_fread(SyscallRegs* regs) {
    int fd = (int)regs->ebx - 3;
    uint8_t* buffer = (uint8_t*)regs->ecx;
    uint32_t size = regs->edx;

    if (fd < 0 || fd >= MAX_OPEN_FILES || !open_files[fd].in_use) return 0;
    KernelFileDesc& f = open_files[fd];
    if (f.mode != FMODE_READ) return 0;

    if (f.offset >= f.size) return 0;
    uint32_t remaining = f.size - f.offset;
    if (size > remaining) size = remaining;

    char orig_name[13];
    int ni = 0;
    for (int i = 0; i < 8 && f.name[i] != ' '; i++) orig_name[ni++] = f.name[i];
    if (f.name[8] != ' ') {
        orig_name[ni++] = '.';
        for (int i = 8; i < 11 && f.name[i] != ' '; i++) orig_name[ni++] = f.name[i];
    }
    orig_name[ni] = '\0';

    int bytes = Fat16::read_file_offset(orig_name, f.offset, buffer, size);
    if (bytes > 0) f.offset += bytes;
    return (uint32_t)(bytes > 0 ? bytes : 0);
}

static uint32_t sys_fwrite(SyscallRegs* regs) {
    int fd = (int)regs->ebx - 3;
    const uint8_t* data = (const uint8_t*)regs->ecx;
    uint32_t size = regs->edx;

    if (fd < 0 || fd >= MAX_OPEN_FILES || !open_files[fd].in_use) return 0;
    KernelFileDesc& f = open_files[fd];
    if (f.mode != FMODE_WRITE) return 0;

    if (!f.write_buf) {
        f.write_cap = (size > 4096) ? size * 2 : 4096;
        f.write_buf = (uint8_t*)PhysicalMemoryManager::alloc_frame();
        if (!f.write_buf) return 0;
        f.write_len = 0;
    }

    for (uint32_t i = 0; i < size; i++) {
        f.write_buf[f.write_len++] = data[i];
    }

    return size;
}

static uint32_t sys_fclose(SyscallRegs* regs) {
    int fd = (int)regs->ebx - 3;
    if (fd < 0 || fd >= MAX_OPEN_FILES || !open_files[fd].in_use) return (uint32_t)-1;
    KernelFileDesc& f = open_files[fd];

    if (f.mode == FMODE_WRITE && f.write_buf && f.write_len > 0) {
        char orig_name[13];
        int ni = 0;
        for (int i = 0; i < 8 && f.name[i] != ' '; i++) orig_name[ni++] = f.name[i];
        if (f.name[8] != ' ') {
            orig_name[ni++] = '.';
            for (int i = 8; i < 11 && f.name[i] != ' '; i++) orig_name[ni++] = f.name[i];
        }
        orig_name[ni] = '\0';
        Fat16::write_file(orig_name, f.write_buf, f.write_len);
    }

    if (f.write_buf) {
        PhysicalMemoryManager::free_frame(f.write_buf);
        f.write_buf = nullptr;
    }

    f.in_use = false;
    return 0;
}

static uint32_t sys_fsize(SyscallRegs* regs) {
    int fd = (int)regs->ebx - 3;
    if (fd < 0 || fd >= MAX_OPEN_FILES || !open_files[fd].in_use) return (uint32_t)-1;
    return open_files[fd].size;
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
    sys_find_thread, // 27
    sys_sbrk,        // 28
    sys_fopen,       // 29
    sys_fread,       // 30
    sys_fwrite,      // 31
    sys_fclose,      // 32
    sys_fsize,       // 33
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
