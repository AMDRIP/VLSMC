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
#include "kernel/elf.h"
#include "kernel/elf_loader.h"
#include "kernel/tss.h"
#include "kernel/kmalloc.h"
#include "kernel/bga.h"
#include "kernel/vga.h"
#include "kernel/rtc.h"
#include "libc.h"

namespace re36 {

Registers* g_current_isr_regs = nullptr;


extern "C" void isr128();

void syscall_gate_init() {
    set_idt_gate(128, (uint32_t)isr128, 0x08, 0xEE);
    // 0xEE = Present(1) | DPL=3(11) | Gate Type=Interrupt(01110)
    // DPL=3 позволяет вызов из Ring 3
}

static uint32_t sys_exit(SyscallRegs* regs) {
    int exit_code = (int)regs->ebx;
    Thread& cur = threads[current_tid];
    cur.exit_code = exit_code;

    if (cur.page_directory_phys != (uint32_t*)VMM::kernel_directory_phys_ &&
        cur.page_directory_phys != nullptr) {
        uint32_t* old_dir = cur.page_directory_phys;
        VMM::switch_address_space((uint32_t*)VMM::kernel_directory_phys_);
        VMM::destroy_address_space(old_dir);
        cur.page_directory_phys = (uint32_t*)VMM::kernel_directory_phys_;
    }

    VMA* v = cur.vma_list;
    while (v) { VMA* n = v->next; kfree(v); v = n; }
    cur.vma_list = nullptr;

    for (int f = 0; f < MAX_OPEN_FILES; f++) {
        if (cur.fd_table[f]) {
            file_release(cur.fd_table[f]);
            cur.fd_table[f] = nullptr;
        }
    }

    if (cur.parent_tid >= 0 && cur.parent_tid < MAX_THREADS &&
        threads[cur.parent_tid].state != ThreadState::Unused) {

        {
            InterruptGuard guard;
            cur.state = ThreadState::Zombie;

            if (threads[cur.parent_tid].state == ThreadState::Blocked &&
                threads[cur.parent_tid].blocked_channel_id == -2) {
                TaskScheduler::unblock(cur.parent_tid);
            }
        }

        int next_tid = TaskScheduler::pick_next_thread();
        if (next_tid != current_tid) {
            InterruptGuard guard;
            int old_tid = current_tid;
            current_tid = next_tid;
            threads[next_tid].state = ThreadState::Running;
            threads[next_tid].quantum_remaining = 5;
            if (threads[next_tid].page_directory_phys != threads[old_tid].page_directory_phys) {
                VMM::switch_address_space(threads[next_tid].page_directory_phys);
            }
            TSS::set_kernel_stack((uint32_t)(threads[next_tid].stack_base + THREAD_STACK_SIZE));
            switch_task(&threads[old_tid].esp, threads[next_tid].esp);
        }
        return 0;
    }

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
    return RTC::to_unix_timestamp();
}

static uint32_t sys_uptime(SyscallRegs* regs) {
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

static uint32_t find_free_vaddr(uint32_t size) {
    uint32_t candidate = 0x20000000;
    uint32_t end_limit = 0xB0000000;
    Thread& cur = threads[current_tid];

    while (candidate + size <= end_limit) {
        bool conflict = false;
        VMA* v = cur.vma_list;
        while (v) {
            if (candidate < v->end && (candidate + size) > v->start) {
                candidate = (v->end + 0xFFF) & ~0xFFF;
                conflict = true;
                break;
            }
            v = v->next;
        }
        if (!conflict) return candidate;
    }
    return 0;
}

static uint32_t sys_mmap(SyscallRegs* regs) {
    uint32_t addr   = regs->ebx;
    uint32_t length = regs->ecx;
    uint32_t prot   = regs->edx;
    uint32_t flags  = regs->esi;
    int      fd     = (int)regs->edi;

    if (length == 0) return (uint32_t)-1;

    length = (length + 0xFFF) & ~0xFFF;

    uint32_t page_flags = PAGE_PRESENT | PAGE_USER;
    if (prot & PROT_WRITE) page_flags |= PAGE_WRITABLE;

    uint32_t vaddr;
    if (addr && (flags & MAP_FIXED)) {
        if (addr < KERNEL_SPACE_END) return (uint32_t)-1;
        vaddr = addr & ~0xFFF;
    } else {
        vaddr = find_free_vaddr(length);
        if (vaddr == 0) return (uint32_t)-1;
    }

    Thread& cur = threads[current_tid];

    VMA* vma = (VMA*)kmalloc(sizeof(VMA));
    if (!vma) return (uint32_t)-1;

    vma->start = vaddr;
    vma->end = vaddr + length;
    vma->flags = page_flags;

    if (flags & MAP_ANONYMOUS) {
        vma->type = VMA_TYPE_ANON;
        vma->file_vnode = nullptr;
        vma->file_offset = 0;
        vma->file_size = 0;

        for (uint32_t off = 0; off < length; off += 4096) {
            void* frame = PhysicalMemoryManager::alloc_frame();
            if (!frame) {
                for (uint32_t undo = 0; undo < off; undo += 4096) {
                    uint32_t phys = VMM::get_physical(vaddr + undo);
                    if (phys) {
                        VMM::unmap_page(vaddr + undo);
                        PhysicalMemoryManager::free_frame((void*)phys);
                    }
                }
                kfree(vma);
                return (uint32_t)-1;
            }
            VMM::map_page(vaddr + off, (uint32_t)frame, page_flags);
            uint8_t* p = (uint8_t*)(vaddr + off);
            for (int b = 0; b < 4096; b++) p[b] = 0;
        }
    } else {
        if (fd < 0 || fd >= MAX_OPEN_FILES || !cur.fd_table[fd]) {
            kfree(vma);
            return (uint32_t)-1;
        }
        file* f = cur.fd_table[fd];
        if (!f->vn) {
            kfree(vma);
            return (uint32_t)-1;
        }
        vma->type = VMA_TYPE_FILE;
        vma->file_vnode = f->vn;
        __atomic_add_fetch(&f->vn->refcount, 1, __ATOMIC_SEQ_CST);
        vma->file_offset = 0;
        vma->file_size = length;
    }

    vma->next = cur.vma_list;
    cur.vma_list = vma;

    return vaddr;
}

static uint32_t sys_munmap(SyscallRegs* regs) {
    uint32_t addr   = regs->ebx;
    uint32_t length = regs->ecx;

    if (addr == 0 || length == 0) return (uint32_t)-1;
    addr &= ~0xFFF;
    length = (length + 0xFFF) & ~0xFFF;

    Thread& cur = threads[current_tid];

    for (uint32_t off = 0; off < length; off += 4096) {
        uint32_t v = addr + off;
        uint32_t phys = VMM::get_physical(v);
        if (phys) {
            VMM::unmap_page(v);
            PhysicalMemoryManager::free_frame((void*)phys);
        }
    }

    VMA** prev = &cur.vma_list;
    while (*prev) {
        VMA* v = *prev;
        if (v->start >= addr && v->end <= addr + length) {
            *prev = v->next;
            if (v->type == VMA_TYPE_FILE && v->file_vnode) {
                vnode_release(v->file_vnode);
            }
            kfree(v);
        } else {
            prev = &v->next;
        }
    }

    return 0;
}

static uint32_t sys_inb(SyscallRegs* regs) {
    if (!threads[current_tid].is_driver) return (uint32_t)-1;
    uint16_t port = (uint16_t)regs->ebx;
    return inb(port);
}

static uint32_t sys_outb(SyscallRegs* regs) {
    if (!threads[current_tid].is_driver) return 0;
    uint16_t port = (uint16_t)regs->ebx;
    uint8_t data = (uint8_t)regs->ecx;
    outb(port, data);
    return 0;
}

static uint32_t sys_inw(SyscallRegs* regs) {
    if (!threads[current_tid].is_driver) return (uint32_t)-1;
    uint16_t port = (uint16_t)regs->ebx;
    return inw(port);
}

static uint32_t sys_outw(SyscallRegs* regs) {
    if (!threads[current_tid].is_driver) return 0;
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

    Thread& cur = threads[current_tid];
    if (!cur.is_driver && cur.tid != 0) {
        printf("[SYSCALL] map_mmio failed: TID %d is not a driver\n", current_tid);
        return 0;
    }

    bool allowed = false;
    if (cur.is_driver) {
        allowed = true;
    } else {
        for (int i = 0; i < cur.num_mmio_grants; i++) {
            if (phys >= cur.allowed_mmio[i].phys_start && 
                (phys + size_pages * 4096 - 1) <= cur.allowed_mmio[i].phys_end) {
                allowed = true;
                break;
            }
        }
    }

    if (!allowed && cur.tid != 0) {
        printf("[SYSCALL] map_mmio failed: phys 0x%x not in MmioGrants for TID %d\n", phys, current_tid);
        return 0;
    }

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
    EventSystem::wait((int)irq);
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

static int find_free_fd(Thread& cur) {
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (!cur.fd_table[i]) return i;
    }
    return -1;
}

// Simplified flag conversion for MVP
static int map_flags(int api_mode) {
    if (api_mode == 1) return O_RDONLY;      // FMODE_READ
    if (api_mode == 2) return O_WRONLY | O_CREAT | O_TRUNC; // FMODE_WRITE
    return O_RDONLY;
}

static uint32_t sys_fopen(SyscallRegs* regs) {
    const char* path = (const char*)regs->ebx;
    int mode = (int)regs->ecx;
    if (!path) return (uint32_t)-1;

    Thread& cur = threads[current_tid];
    int fd_idx = find_free_fd(cur);
    if (fd_idx < 0) return (uint32_t)-1;

    int vfs_flags = map_flags(mode);
    int vn_ptr = vfs_open(path, vfs_flags, 0); // VFS returns vnode* cast to int as a hack
    if (vn_ptr == -1) return (uint32_t)-1;
    
    vnode* vn = (vnode*)vn_ptr;

    file* f = (file*)kmalloc(sizeof(file));
    if (!f) {
        if (vn->ops && vn->ops->close) vn->ops->close(vn);
        return (uint32_t)-1;
    }

    f->vn = vn;
    f->offset = 0;
    f->flags = (uint32_t)vfs_flags;
    f->refcount = 1;
    
    cur.fd_table[fd_idx] = f;

    // +3 offset since 0,1,2 are reserved
    return (uint32_t)(fd_idx + 3);
}

static uint32_t sys_fread(SyscallRegs* regs) {
    int fd = (int)regs->ebx - 3;
    uint8_t* buffer = (uint8_t*)regs->ecx;
    uint32_t size = regs->edx;

    if (fd < 0 || fd >= MAX_OPEN_FILES) return 0;
    
    Thread& cur = threads[current_tid];
    file* f = cur.fd_table[fd];
    if (!f || !f->vn) return 0;

    if ((f->flags & O_WRONLY) && !(f->flags & O_RDWR)) return 0;

    int read_bytes = -1;
    if (f->vn->ops && f->vn->ops->read) {
        read_bytes = f->vn->ops->read(f->vn, f->offset, buffer, size);
    }

    if (read_bytes > 0) {
        f->offset += read_bytes;
        return (uint32_t)read_bytes;
    }
    
    return 0;
}

static uint32_t sys_fwrite(SyscallRegs* regs) {
    int fd = (int)regs->ebx - 3;
    const uint8_t* data = (const uint8_t*)regs->ecx;
    uint32_t size = regs->edx;

    if (fd < 0 || fd >= MAX_OPEN_FILES) return 0;
    
    Thread& cur = threads[current_tid];
    file* f = cur.fd_table[fd];
    if (!f || !f->vn) return 0;

    int written = -1;
    if (f->vn->ops && f->vn->ops->write) {
        written = f->vn->ops->write(f->vn, f->offset, data, size);
    }
    
    if (written > 0) {
        f->offset += written;
        return (uint32_t)written;
    }

    return 0;
}

static uint32_t sys_fclose(SyscallRegs* regs) {
    int fd = (int)regs->ebx - 3;
    if (fd < 0 || fd >= MAX_OPEN_FILES) return (uint32_t)-1;
    
    Thread& cur = threads[current_tid];
    file* f = cur.fd_table[fd];
    if (!f) return (uint32_t)-1;

    file_release(f);

    cur.fd_table[fd] = nullptr;
    return 0;
}

static uint32_t sys_fsize(SyscallRegs* regs) {
    int fd = (int)regs->ebx - 3;
    if (fd < 0 || fd >= MAX_OPEN_FILES) return (uint32_t)-1;
    
    Thread& cur = threads[current_tid];
    file* f = cur.fd_table[fd];
    if (!f || !f->vn) return (uint32_t)-1;

    return f->vn->size;
}
static void fork_child_entry() {
    TSS::set_kernel_stack((uint32_t)(threads[current_tid].stack_base + THREAD_STACK_SIZE));

    ForkChildState* r = &threads[current_tid].fork_state;

    asm volatile(
        "cli\n\t"
        "mov $0x23, %%cx\n\t"
        "mov %%cx, %%ds\n\t"
        "mov %%cx, %%es\n\t"
        "mov %%cx, %%fs\n\t"
        "mov %%cx, %%gs\n\t"
        "push $0x23\n\t"
        "push 4(%%eax)\n\t"
        "push 8(%%eax)\n\t"
        "push $0x1B\n\t"
        "push 0(%%eax)\n\t"
        "mov 12(%%eax), %%ebx\n\t"
        "mov 20(%%eax), %%edx\n\t"
        "mov 24(%%eax), %%esi\n\t"
        "mov 28(%%eax), %%edi\n\t"
        "mov 32(%%eax), %%ebp\n\t"
        "mov 16(%%eax), %%ecx\n\t"
        "xor %%eax, %%eax\n\t"
        "iret\n\t"
        :: "a"(r)
        : "memory"
    );
}

static uint32_t sys_fork(SyscallRegs* regs) {
    (void)regs;
    InterruptGuard guard;

    if (!g_current_isr_regs) return (uint32_t)-1;

    int child_tid = -1;
    for (int i = 1; i < MAX_THREADS; i++) {
        if (threads[i].state == ThreadState::Unused) {
            child_tid = i;
            break;
        }
    }
    if (child_tid == -1) return (uint32_t)-1;

    Thread& parent = threads[current_tid];
    Thread& child = threads[child_tid];

    for (int j = 0; j < 32; j++) child.name[j] = parent.name[j];

    child.fork_state.eip = g_current_isr_regs->eip;
    child.fork_state.useresp = g_current_isr_regs->useresp;
    child.fork_state.eflags = g_current_isr_regs->eflags | 0x200;
    child.fork_state.ebx = g_current_isr_regs->ebx;
    child.fork_state.ecx = g_current_isr_regs->ecx;
    child.fork_state.edx = g_current_isr_regs->edx;
    child.fork_state.esi = g_current_isr_regs->esi;
    child.fork_state.edi = g_current_isr_regs->edi;
    child.fork_state.ebp = g_current_isr_regs->ebp;

    child.tid = child_tid;
    child.state = ThreadState::Ready;
    child.priority = parent.priority;
    child.sleep_until = 0;
    child.blocked_channel_id = -1;
    child.quantum_remaining = 5;
    child.total_ticks = 0;
    child.msg_head = 0;
    child.msg_tail = 0;
    child.msg_count = 0;
    child.waiting_for_msg = false;
    child.parent_tid = current_tid;
    child.exit_code = 0;
    child.heap_start = parent.heap_start;
    child.heap_end = parent.heap_end;
    child.heap_lock = false;
    child.is_driver = false;
    child.num_mmio_grants = 0;

    child.vma_list = nullptr;
    VMA* src_vma = parent.vma_list;
    VMA** dst_ptr = &child.vma_list;
    while (src_vma) {
        VMA* copy = (VMA*)kmalloc(sizeof(VMA));
        if (!copy) break;
        copy->start = src_vma->start;
        copy->end = src_vma->end;
        copy->file_offset = src_vma->file_offset;
        copy->file_size = src_vma->file_size;
        copy->flags = src_vma->flags;
        copy->next = nullptr;
        *dst_ptr = copy;
        dst_ptr = &copy->next;
        src_vma = src_vma->next;
    }
    
    // Inherit file descriptors
    for (int f = 0; f < MAX_OPEN_FILES; f++) {
        if (parent.fd_table[f]) {
            child.fd_table[f] = parent.fd_table[f];
            __atomic_add_fetch(&child.fd_table[f]->refcount, 1, __ATOMIC_SEQ_CST);
            if (child.fd_table[f]->vn) {
                __atomic_add_fetch(&child.fd_table[f]->vn->refcount, 1, __ATOMIC_SEQ_CST);
            }
        } else {
            child.fd_table[f] = nullptr;
        }
    }

    uint32_t* new_dir = VMM::clone_directory();
    if (!new_dir) {
        for (int f = 0; f < MAX_OPEN_FILES; f++) {
            if (child.fd_table[f]) {
                file_release(child.fd_table[f]);
                child.fd_table[f] = nullptr;
            }
        }
        VMA* rv = child.vma_list;
        while (rv) { VMA* rn = rv->next; kfree(rv); rv = rn; }
        child.vma_list = nullptr;
        child.state = ThreadState::Unused;
        return (uint32_t)-1;
    }
    child.page_directory_phys = new_dir;

    uint32_t* stack_top = (uint32_t*)(child.stack_base + THREAD_STACK_SIZE);

    *(--stack_top) = (uint32_t)fork_child_entry;

    *(--stack_top) = 0x202;
    *(--stack_top) = 0;
    *(--stack_top) = 0;
    *(--stack_top) = 0;
    *(--stack_top) = 0;

    child.esp = (uint32_t)stack_top;

    thread_count++;
    return (uint32_t)child_tid;
}

static uint32_t sys_exec(SyscallRegs* regs) {
    const char* filename = (const char*)regs->ebx;
    char* const* argv = (char* const*)regs->ecx;
    char* const* envp = (char* const*)regs->edx;

    if (!filename) return (uint32_t)-1;

    Thread& cur = threads[current_tid];

    for (int j = 0; j < 31 && filename[j]; j++) {
        cur.name[j] = filename[j];
        cur.name[j + 1] = '\0';
    }

    uint8_t* header_buf = (uint8_t*)kmalloc(4096);
    if (!header_buf) return (uint32_t)-1;

    vnode* vn = nullptr;
    if (vfs_resolve_path(filename, &vn) != 0 || !vn) {
        kfree(header_buf);
        return (uint32_t)-1;
    }

    int bytes = -1;
    if (vn->ops && vn->ops->read) {
        bytes = vn->ops->read(vn, 0, header_buf, 4096);
    }
    vnode_release(vn);

    if (bytes < (int)sizeof(Elf32_Ehdr)) {
        kfree(header_buf);
        return (uint32_t)-1;
    }

    Elf32_Ehdr* ehdr = (Elf32_Ehdr*)header_buf;
    if (ehdr->e_ident[0] != ELFMAG0 || ehdr->e_ident[1] != ELFMAG1 ||
        ehdr->e_ident[2] != ELFMAG2 || ehdr->e_ident[3] != ELFMAG3) {
        kfree(header_buf);
        return (uint32_t)-1;
    }

    // Подсчет аргументов и копирование строк во временный буфер ядра
    int argc = 0;
    int envc = 0;
    uint32_t total_string_size = 0;

    if (argv) {
        while (argv[argc]) {
            const char* arg = argv[argc];
            int len = 0;
            while (arg[len]) len++;
            total_string_size += len + 1;
            argc++;
        }
    }

    if (envp) {
        while (envp[envc]) {
            const char* env = envp[envc];
            int len = 0;
            while (env[len]) len++;
            total_string_size += len + 1;
            envc++;
        }
    }

    // Выделяем буфер в ядре под строки (не включая массивы указателей)
    uint8_t* string_buf = nullptr;
    if (total_string_size > 0) {
        string_buf = (uint8_t*)kmalloc(total_string_size);
        if (!string_buf) {
            kfree(header_buf);
            return (uint32_t)-1;
        }

        uint32_t offset = 0;
        if (argv) {
            for (int i = 0; i < argc; i++) {
                const char* arg = argv[i];
                int len = 0;
                while (arg[len]) {
                    string_buf[offset++] = arg[len];
                    len++;
                }
                string_buf[offset++] = '\0';
            }
        }
        if (envp) {
            for (int i = 0; i < envc; i++) {
                const char* env = envp[i];
                int len = 0;
                while (env[len]) {
                    string_buf[offset++] = env[len];
                    len++;
                }
                string_buf[offset++] = '\0';
            }
        }
    }

    // Закрываем файлы только с флагом O_CLOEXEC
    for (int f = 0; f < MAX_OPEN_FILES; f++) {
        if (cur.fd_table[f] && (cur.fd_table[f]->flags & O_CLOEXEC)) {
            file_release(cur.fd_table[f]);
            cur.fd_table[f] = nullptr;
        }
    }

    if (cur.page_directory_phys != (uint32_t*)VMM::kernel_directory_phys_) {
        uint32_t* old_dir = cur.page_directory_phys;
        VMM::switch_address_space((uint32_t*)VMM::kernel_directory_phys_);
        VMM::destroy_address_space(old_dir);
    }

    VMA* v = cur.vma_list;
    while (v) {
        VMA* next = v->next;
        kfree(v);
        v = next;
    }
    cur.vma_list = nullptr;

    uint32_t* new_dir = VMM::create_address_space();
    if (!new_dir) {
        if (string_buf) kfree(string_buf);
        kfree(header_buf);
        return (uint32_t)-1;
    }
    cur.page_directory_phys = new_dir;
    VMM::switch_address_space(new_dir);

    Elf32_Phdr* phdrs = (Elf32_Phdr*)(header_buf + ehdr->e_phoff);
    uint32_t max_vaddr = 0;

    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdrs[i].p_type != PT_LOAD || phdrs[i].p_memsz == 0) continue;

        uint32_t vaddr_start = phdrs[i].p_vaddr & ~0xFFF;
        uint32_t vaddr_end = (phdrs[i].p_vaddr + phdrs[i].p_memsz + 0xFFF) & ~0xFFF;
        if (vaddr_end > max_vaddr) max_vaddr = vaddr_end;

        uint32_t flags = PAGE_PRESENT | PAGE_USER;
        if (phdrs[i].p_flags & PF_W) flags |= PAGE_WRITABLE;

        VMA* new_vma = (VMA*)kmalloc(sizeof(VMA));
        if (!new_vma) break;
        new_vma->start = vaddr_start;
        new_vma->end = vaddr_end;
        uint32_t align_diff = phdrs[i].p_vaddr - vaddr_start;
        new_vma->file_offset = phdrs[i].p_offset - align_diff;
        new_vma->file_size = phdrs[i].p_filesz + align_diff;
        new_vma->flags = flags;
        new_vma->type = VMA_TYPE_FILE;      // Set the type
        
        vnode* exec_vn = nullptr;
        if (vfs_resolve_path(cur.name, &exec_vn) == 0 && exec_vn) {
            new_vma->file_vnode = exec_vn;  // Store vnode for demand paging
        } else {
            new_vma->file_vnode = nullptr;
        }

        new_vma->next = cur.vma_list;
        cur.vma_list = new_vma;
    }

    uint32_t entry = ehdr->e_entry;
    kfree(header_buf);

    uint32_t heap_base = (max_vaddr + 0xFFF) & ~0xFFF;
    heap_base += 4096;
    cur.heap_start = heap_base;
    cur.heap_end = heap_base;
    cur.heap_lock = false;

    for (uint32_t p = 0; p < USER_STACK_PAGES; p++) {
        void* frame = PhysicalMemoryManager::alloc_frame();
        if (!frame) return (uint32_t)-1; // TODO: handle rollback correctly
        uint32_t vaddr = USER_STACK_TOP - (USER_STACK_PAGES - p) * 4096;
        VMM::map_page(vaddr, (uint32_t)frame, PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);
        uint8_t* pp = (uint8_t*)vaddr;
        for (int b = 0; b < 4096; b++) pp[b] = 0;
    }

    uint32_t user_esp = USER_STACK_TOP;

    // Раскладка стека The System V i386 ABI
    // [Строки ARG и ENV]
    // [NULL]
    // [ENV Pointers]
    // [NULL]
    // [ARG Pointers]
    // [argc] <-- ESP

    if (total_string_size > 0) {
        user_esp -= total_string_size;
        user_esp &= ~3; // Выравнивание по 4 байта

        uint8_t* stack_strings = (uint8_t*)user_esp;
        for (uint32_t k = 0; k < total_string_size; k++) {
            stack_strings[k] = string_buf[k];
        }

        kfree(string_buf);
        
        // Массив envp (pointers + NULL terminator)
        user_esp -= (envc + 1) * sizeof(char*);
        char** user_envp = (char**)user_esp;
        
        // Массив argv (pointers + NULL terminator)
        user_esp -= (argc + 1) * sizeof(char*);
        char** user_argv = (char**)user_esp;
        
        uint32_t str_offset = (uint32_t)stack_strings;

        for (int i = 0; i < argc; i++) {
            user_argv[i] = (char*)str_offset;
            while (*(char*)str_offset) str_offset++;
            str_offset++; // Skip \0
        }
        user_argv[argc] = nullptr;

        for (int i = 0; i < envc; i++) {
            user_envp[i] = (char*)str_offset;
            while (*(char*)str_offset) str_offset++;
            str_offset++; // Skip \0
        }
        user_envp[envc] = nullptr;
    } else {
        // Нет строк (argc=0, envc=0)
        user_esp -= sizeof(char*);
        char** user_envp = (char**)user_esp;
        user_envp[0] = nullptr;
        
        user_esp -= sizeof(char*);
        char** user_argv = (char**)user_esp;
        user_argv[0] = nullptr;
    }

    // Push argc
    user_esp -= sizeof(int);
    *(int*)user_esp = argc;
    TSS::set_kernel_stack((uint32_t)(cur.stack_base + THREAD_STACK_SIZE));

    uint32_t eflags;
    asm volatile("pushf; pop %0" : "=r"(eflags));
    eflags |= 0x200;

    asm volatile(
        "cli\n\t"
        "mov $0x23, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        "push $0x23\n\t"
        "push %%ecx\n\t"
        "push %%ebx\n\t"
        "push $0x1B\n\t"
        "push %%edx\n\t"
        "iret\n\t"
        :: "c"(user_esp), "d"(entry), "b"(eflags)
        : "eax", "memory"
    );

    return 0;
}

static uint32_t sys_wait(SyscallRegs* regs) {
    int* status_ptr = (int*)regs->ebx;

    while (true) {
        {
            InterruptGuard guard;

            bool has_children = false;
            for (int i = 1; i < MAX_THREADS; i++) {
                if (threads[i].parent_tid == current_tid) {
                    has_children = true;
                    if (threads[i].state == ThreadState::Zombie) {
                        int child_tid = i;
                        if (status_ptr) *status_ptr = threads[i].exit_code;
                        threads[i].state = ThreadState::Unused;
                        thread_count--;
                        return (uint32_t)child_tid;
                    }
                }
            }

            if (!has_children) return (uint32_t)-1;
        }

        TaskScheduler::block_current(-2);
    }
}

static uint32_t sys_grant_mmio(SyscallRegs* regs) {
    int target_tid = (int)regs->ebx;
    uint32_t phys_start = regs->ecx;
    uint32_t phys_end = regs->edx;

    Thread& cur = threads[current_tid];
    if (!cur.is_driver) return (uint32_t)-1;

    if (target_tid < 0 || target_tid >= MAX_THREADS) return (uint32_t)-1;
    Thread& target = threads[target_tid];
    if (target.state == ThreadState::Unused) return (uint32_t)-1;

    if (target.num_mmio_grants >= 8) return (uint32_t)-1;

    target.allowed_mmio[target.num_mmio_grants].phys_start = phys_start;
    target.allowed_mmio[target.num_mmio_grants].phys_end = phys_end;
    target.num_mmio_grants++;

    return 0;
}

static uint32_t sys_set_driver(SyscallRegs* regs) {
    int target_tid = (int)regs->ebx;

    Thread& cur = threads[current_tid];
    if (!cur.is_driver) return (uint32_t)-1;

    if (target_tid < 0 || target_tid >= MAX_THREADS) return (uint32_t)-1;
    Thread& target = threads[target_tid];
    if (target.state == ThreadState::Unused) return (uint32_t)-1;

    target.is_driver = true;
    return 0;
}

static uint32_t sys_unmap_mmio(SyscallRegs* regs) {
    uint32_t virt_addr = regs->ebx;
    uint32_t size_pages = regs->ecx;
    Thread& cur = threads[current_tid];

    if (!cur.is_driver) return (uint32_t)-1;
    if (virt_addr < 0x20000000 || virt_addr >= 0xC0000000) return (uint32_t)-1;

    for (uint32_t i = 0; i < size_pages; i++) {
        VMM::unmap_page(virt_addr + i * 4096);
    }
    
    // Remove from VMA list
    VMA** pp = &cur.vma_list;
    while (*pp) {
        if ((*pp)->start == virt_addr) {
            VMA* to_del = *pp;
            *pp = to_del->next;
            kfree(to_del);
            break;
        }
        pp = &(*pp)->next;
    }
    return 0;
}

static uint32_t sys_get_vga_info(SyscallRegs* regs) {
    uint32_t* width_out = (uint32_t*)regs->ebx;
    uint32_t* height_out = (uint32_t*)regs->ecx;
    uint32_t* bpp_out = (uint32_t*)regs->edx;
    uint32_t* phys_addr_out = (uint32_t*)regs->esi;

    if (width_out) *width_out = BgaDriver::is_initialized() ? BgaDriver::get_width() : 320;
    if (height_out) *height_out = BgaDriver::is_initialized() ? BgaDriver::get_height() : 200;
    if (bpp_out) *bpp_out = BgaDriver::is_initialized() ? BgaDriver::get_bpp() : 8;
    if (phys_addr_out) *phys_addr_out = BgaDriver::is_initialized() ? BgaDriver::get_lfb() : 0xA0000;

    return 0;
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
    sys_fork,        // 34
    sys_exec,        // 35
    sys_wait,        // 36
    sys_grant_mmio,  // 37
    sys_set_driver,  // 38
    sys_unmap_mmio,  // 39
    sys_get_vga_info,// 40
    sys_uptime,      // 41
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
