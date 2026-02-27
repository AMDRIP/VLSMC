#pragma once

#include <stdint.h>
#include <stddef.h>

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

struct VMA {
    uint32_t start;
    uint32_t end;
    uint32_t file_offset;
    uint32_t file_size;
    uint32_t flags;
    VMA* next;
};

struct Thread {
    uint32_t tid;
    char name[32];
    
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
