#include "kernel/task_scheduler.h"
#include "kernel/timer.h"
#include "kernel/spinlock.h"
#include "kernel/tss.h"
#include "kernel/vmm.h"
#include "libc.h"

namespace re36 {

bool TaskScheduler::scheduling_enabled_ = false;

#define AGING_INTERVAL 50
#define AGING_BOOST 1

void TaskScheduler::init() {
    thread_init();
    scheduling_enabled_ = true;
}

int TaskScheduler::pick_next_thread() {
    int best_tid = 0;
    uint8_t best_priority = 255;

    uint32_t now = Timer::get_ticks();

    for (int i = 0; i < MAX_THREADS; i++) {
        if (threads[i].state == ThreadState::Sleeping) {
            if (now >= threads[i].sleep_until) {
                threads[i].state = ThreadState::Ready;
                threads[i].blocked_channel_id = -1;
            }
        }
    }

    for (int i = 0; i < MAX_THREADS; i++) {
        if (i == current_tid) continue;
        if (threads[i].state == ThreadState::Ready) {
            if (threads[i].priority < best_priority) {
                best_priority = threads[i].priority;
                best_tid = i;
            }
        }
    }

    if (best_tid == 0 && threads[current_tid].state == ThreadState::Running) {
        return current_tid;
    }

    return best_tid;
}

void TaskScheduler::schedule() {
    if (!scheduling_enabled_) return;

    InterruptGuard guard;

    static uint32_t aging_counter = 0;
    aging_counter++;
    if (aging_counter >= AGING_INTERVAL) {
        aging_counter = 0;
        for (int i = 1; i < MAX_THREADS; i++) {
            if (threads[i].state == ThreadState::Ready && threads[i].priority > 1) {
                threads[i].priority -= AGING_BOOST;
            }
        }
    }

    Thread& cur = threads[current_tid];
    
    if (cur.state == ThreadState::Running) {
        cur.quantum_remaining--;
        cur.total_ticks++;
        
        if (cur.quantum_remaining > 0) {
            return;
        }
        
        cur.state = ThreadState::Ready;
        cur.quantum_remaining = DEFAULT_QUANTUM;
    }

    int next_tid = pick_next_thread();
    
    if (next_tid == current_tid && cur.state == ThreadState::Ready) {
        cur.state = ThreadState::Running;
        cur.quantum_remaining = DEFAULT_QUANTUM;
        return;
    }
    
    if (next_tid == current_tid) return;

    int old_tid = current_tid;
    current_tid = next_tid;
    
    threads[next_tid].state = ThreadState::Running;
    threads[next_tid].quantum_remaining = DEFAULT_QUANTUM;

    if (threads[next_tid].page_directory_phys != threads[old_tid].page_directory_phys) {
        VMM::switch_address_space(threads[next_tid].page_directory_phys);
    }

    TSS::set_kernel_stack((uint32_t)(threads[next_tid].stack_base + THREAD_STACK_SIZE));

    switch_task(&threads[old_tid].esp, threads[next_tid].esp);
}

void TaskScheduler::block_current(int channel_id) {
    InterruptGuard guard;
    threads[current_tid].state = ThreadState::Blocked;
    threads[current_tid].blocked_channel_id = channel_id;
    
    int next_tid = pick_next_thread();
    if (next_tid != current_tid) {
        int old_tid = current_tid;
        current_tid = next_tid;
        threads[next_tid].state = ThreadState::Running;
        threads[next_tid].quantum_remaining = DEFAULT_QUANTUM;
        
        if (threads[next_tid].page_directory_phys != threads[old_tid].page_directory_phys) {
            VMM::switch_address_space(threads[next_tid].page_directory_phys);
        }

        switch_task(&threads[old_tid].esp, threads[next_tid].esp);
    }
}

void TaskScheduler::unblock(int tid) {
    InterruptGuard guard;
    if (tid < 0 || tid >= MAX_THREADS) return;
    if (threads[tid].state == ThreadState::Blocked) {
        threads[tid].state = ThreadState::Ready;
        threads[tid].blocked_channel_id = -1;
    }
}

void TaskScheduler::sleep_current(uint32_t ms) {
    InterruptGuard guard;
    uint32_t ticks_to_sleep = (ms * 100) / 1000;
    if (ticks_to_sleep == 0) ticks_to_sleep = 1;
    
    threads[current_tid].state = ThreadState::Sleeping;
    threads[current_tid].sleep_until = Timer::get_ticks() + ticks_to_sleep;
    
    int next_tid = pick_next_thread();
    if (next_tid != current_tid) {
        int old_tid = current_tid;
        current_tid = next_tid;
        threads[next_tid].state = ThreadState::Running;
        threads[next_tid].quantum_remaining = DEFAULT_QUANTUM;
        
        if (threads[next_tid].page_directory_phys != threads[old_tid].page_directory_phys) {
            VMM::switch_address_space(threads[next_tid].page_directory_phys);
        }

        switch_task(&threads[old_tid].esp, threads[next_tid].esp);
    }
}

void TaskScheduler::terminate_current() {
    InterruptGuard guard;
    threads[current_tid].state = ThreadState::Terminated;
    
    int next_tid = pick_next_thread();
    if (next_tid != current_tid) {
        int old_tid = current_tid;
        current_tid = next_tid;
        threads[next_tid].state = ThreadState::Running;
        threads[next_tid].quantum_remaining = DEFAULT_QUANTUM;
        
        if (threads[next_tid].page_directory_phys != threads[old_tid].page_directory_phys) {
            VMM::switch_address_space(threads[next_tid].page_directory_phys);
        }

        switch_task(&threads[old_tid].esp, threads[next_tid].esp);
    }
}

int TaskScheduler::get_current_tid() {
    return current_tid;
}

void TaskScheduler::print_threads() {
    InterruptGuard guard;
    
    const char* state_names[] = {
        "Unused", "Ready", "Running", "Blocked", "Sleeping", "Dead"
    };
    
    printf("\n TID | Name              | State    | Pri | Ticks\n");
    printf("-----+-------------------+----------+-----+------\n");
    
    for (int i = 0; i < MAX_THREADS; i++) {
        if (threads[i].state == ThreadState::Unused) continue;
        
        printf(" %d   | %s\t\t| %s\t| %d\t| %d\n",
            threads[i].tid,
            threads[i].name,
            state_names[(int)threads[i].state],
            threads[i].priority,
            threads[i].total_ticks);
    }
    printf("\n");
}

} // namespace re36
