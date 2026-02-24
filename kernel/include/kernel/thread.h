#pragma once

#include <stdint.h>
#include <stddef.h>

namespace re36 {

#define MAX_THREADS 32
#define THREAD_STACK_SIZE 4096

enum class ThreadState : uint8_t {
    Unused = 0,
    Ready,
    Running,
    Blocked,
    Sleeping,
    Terminated
};

typedef void (*ThreadEntry)();

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
};

extern Thread threads[MAX_THREADS];
extern int current_tid;
extern int thread_count;

void thread_init();
int thread_create(const char* name, ThreadEntry entry, uint8_t priority);
void thread_terminate(int tid);
void thread_yield();

extern "C" void switch_task(uint32_t* old_esp, uint32_t new_esp);

} // namespace re36
