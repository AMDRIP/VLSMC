#pragma once

#include <stdint.h>

namespace re36 {

#define MAX_CHANNELS 32
#define CHANNEL_BUFFER_SIZE 64
#define MAX_WAITERS 8

struct EventChannel {
    bool active;
    char name[32];
    
    uint32_t buffer[CHANNEL_BUFFER_SIZE];
    int head;
    int tail;
    
    int waiting_tids[MAX_WAITERS];
    int waiter_count;
};

class EventSystem {
public:
    static void init();
    
    static int create_channel(const char* name);
    
    static void destroy_channel(int channel_id);
    
    static bool push(int channel_id, uint32_t event_data);
    
    static uint32_t pop(int channel_id);
    
    static uint32_t wait(int channel_id);

private:
    static EventChannel channels_[MAX_CHANNELS];
};

} // namespace re36
