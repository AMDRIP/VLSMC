#pragma once

#include <stdint.h>
#include "kernel/thread.h"

namespace re36 {

#define DEFAULT_QUANTUM 5 // 5 тиков = 50 мс при 100 Hz

class TaskScheduler {
public:
    static void init();
    
    static void schedule();
    
    static void block_current(int channel_id);
    
    static void unblock(int tid);
    
    static void sleep_current(uint32_t ms);
    
    static void terminate_current();
    
    static int get_current_tid();
    
    static void join(int tid);
    
    static void print_threads();
    
    static int pick_next_thread();

private:
    static bool scheduling_enabled_;
};

} // namespace re36
