#include "kernel/thread.h"
#include "kernel/spinlock.h"
#include "libc.h"

namespace re36 {

Thread threads[MAX_THREADS];
int current_tid = 0;
int thread_count = 0;

static uint8_t thread_stacks[MAX_THREADS][THREAD_STACK_SIZE] __attribute__((aligned(16)));

static void thread_exit_wrapper() {
    printf("\n[Thread %d terminated]\n", current_tid);
    threads[current_tid].state = ThreadState::Terminated;
    thread_yield();
    while (true) asm volatile("hlt");
}

void thread_init() {
    for (int i = 0; i < MAX_THREADS; i++) {
        threads[i].tid = i;
        threads[i].state = ThreadState::Unused;
        threads[i].priority = 255;
        threads[i].esp = 0;
        threads[i].stack_base = thread_stacks[i];
        threads[i].sleep_until = 0;
        threads[i].blocked_channel_id = -1;
        threads[i].quantum_remaining = 5;
        threads[i].total_ticks = 0;
        threads[i].name[0] = '\0';
    }

    threads[0].state = ThreadState::Running;
    threads[0].priority = 128;
    threads[0].name[0] = 'b'; threads[0].name[1] = 'o';
    threads[0].name[2] = 'o'; threads[0].name[3] = 't';
    threads[0].name[4] = '\0';
    
    current_tid = 0;
    thread_count = 1;
}

int thread_create(const char* name, ThreadEntry entry, uint8_t priority) {
    InterruptGuard guard;
    
    int tid = -1;
    for (int i = 1; i < MAX_THREADS; i++) {
        if (threads[i].state == ThreadState::Unused) {
            tid = i;
            break;
        }
    }
    if (tid == -1) return -1;

    Thread& t = threads[tid];
    
    int j = 0;
    while (name[j] && j < 31) {
        t.name[j] = name[j];
        j++;
    }
    t.name[j] = '\0';
    
    t.state = ThreadState::Ready;
    t.priority = priority;
    t.sleep_until = 0;
    t.blocked_channel_id = -1;
    t.quantum_remaining = 5;
    t.total_ticks = 0;

    uint32_t* stack_top = (uint32_t*)(t.stack_base + THREAD_STACK_SIZE);

    *(--stack_top) = (uint32_t)thread_exit_wrapper;
    *(--stack_top) = (uint32_t)entry;

    *(--stack_top) = 0x202;  // EFLAGS (IF=1)
    *(--stack_top) = 0;      // EBX
    *(--stack_top) = 0;      // ESI
    *(--stack_top) = 0;      // EDI
    *(--stack_top) = 0;      // EBP

    t.esp = (uint32_t)stack_top;
    
    thread_count++;
    return tid;
}

void thread_terminate(int tid) {
    if (tid < 0 || tid >= MAX_THREADS) return;
    threads[tid].state = ThreadState::Terminated;
}

void thread_yield() {
    asm volatile("int $0x20"); // IRQ0
}

} // namespace re36
