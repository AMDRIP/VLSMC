#pragma once

#include <stdint.h>
#include <stddef.h>
#include "kernel/vfs.h"

namespace re36 {

struct ForkChildState {
    uint32_t eip;
    uint32_t useresp;
    uint32_t eflags;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
};

#define MAX_THREADS 32
#define THREAD_STACK_SIZE 4096

#define IPC_MAX_MSG_SIZE 512
#define IPC_MSG_QUEUE_SIZE 4

#define MAX_OPEN_FILES 16

struct IpcMessage {
    int sender_tid;
    uint32_t size;
    uint8_t data[IPC_MAX_MSG_SIZE];
};

enum class ThreadState : uint8_t {
    Unused = 0,
    Ready,
    Running,
    Blocked,
    Sleeping,
    Terminated,
    Zombie
};

typedef void (*ThreadEntry)();

#define VMA_TYPE_ANON  0
#define VMA_TYPE_FILE  1

#define MAP_PRIVATE    0x02
#define MAP_ANONYMOUS  0x20
#define MAP_FIXED      0x10

#define PROT_NONE  0x0
#define PROT_READ  0x1
#define PROT_WRITE 0x2
#define PROT_EXEC  0x4

struct vnode;

struct VMA {
    uint32_t start;
    uint32_t end;
    uint32_t file_offset;
    uint32_t file_size;
    uint32_t flags;
    uint8_t  type;
    vnode*   file_vnode;
    VMA* next;
};

struct MmioGrant {
    uint32_t phys_start;
    uint32_t phys_end;
};

struct Thread {
    uint32_t tid;
    char name[32];
    
    bool is_driver;
    int num_mmio_grants;
    MmioGrant allowed_mmio[8];
    
    ThreadState state;
    uint8_t priority;       // 0 = наивысший, 255 = идле
    
    uint32_t esp;           // Сохраненный указатель стека
    uint8_t* stack_base;    // Начало выделенного стека
    
    uint32_t sleep_until;   // Тик пробуждения (если Sleeping)
    int blocked_channel_id; // ID канала, если заблокирован (-1 = нет)
    
    uint32_t quantum_remaining; // Остаток кванта
    uint32_t total_ticks;       // Всего тиков процессорного времени
    
    uint32_t* page_directory_phys; // Каталог страниц потока
    uint32_t heap_start;        // Начало кучи (после кода)
    uint32_t heap_end;          // Текущий конец кучи
    bool heap_lock;             // Спинлок для кучи

    VMA* vma_list;              // Динамический список виртуальной памяти (Demand Paging / mmap)

    IpcMessage messages[IPC_MSG_QUEUE_SIZE];
    int msg_head;
    int msg_tail;
    int msg_count;
    bool waiting_for_msg;

    int parent_tid;
    int exit_code;

    file* fd_table[MAX_OPEN_FILES]; // VFS file descriptors for this thread

    ForkChildState fork_state;
};

extern Thread threads[MAX_THREADS];
extern int current_tid;
extern int thread_count;

void thread_init();
int thread_create(const char* name, ThreadEntry entry, uint8_t priority);
void thread_terminate(int tid);
void thread_cleanup(int tid);
void thread_yield();

extern "C" void switch_task(uint32_t* old_esp, uint32_t new_esp);

} // namespace re36
