#include "kernel/event_channel.h"
#include "kernel/task_scheduler.h"
#include "kernel/thread.h"
#include "kernel/spinlock.h"
#include "libc.h"

namespace re36 {

EventChannel EventSystem::channels_[MAX_CHANNELS];

void EventSystem::init() {
    for (int i = 0; i < MAX_CHANNELS; i++) {
        channels_[i].active = false;
        channels_[i].head = 0;
        channels_[i].tail = 0;
        channels_[i].waiter_count = 0;
        channels_[i].name[0] = '\0';
    }
}

int EventSystem::create_channel(const char* name) {
    InterruptGuard guard;
    
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (!channels_[i].active) {
            channels_[i].active = true;
            channels_[i].head = 0;
            channels_[i].tail = 0;
            channels_[i].waiter_count = 0;
            
            int j = 0;
            while (name[j] && j < 31) {
                channels_[i].name[j] = name[j];
                j++;
            }
            channels_[i].name[j] = '\0';
            
            return i;
        }
    }
    return -1;
}

void EventSystem::destroy_channel(int channel_id) {
    InterruptGuard guard;
    
    if (channel_id < 0 || channel_id >= MAX_CHANNELS) return;
    
    EventChannel& ch = channels_[channel_id];
    
    for (int i = 0; i < ch.waiter_count; i++) {
        TaskScheduler::unblock(ch.waiting_tids[i]);
    }
    ch.waiter_count = 0;
    ch.active = false;
}

bool EventSystem::push(int channel_id, uint32_t event_data) {
    InterruptGuard guard;
    
    if (channel_id < 0 || channel_id >= MAX_CHANNELS) return false;
    EventChannel& ch = channels_[channel_id];
    if (!ch.active) return false;
    
    int next_head = (ch.head + 1) % CHANNEL_BUFFER_SIZE;
    if (next_head == ch.tail) {
        ch.tail = (ch.tail + 1) % CHANNEL_BUFFER_SIZE;
    }
    
    ch.buffer[ch.head] = event_data;
    ch.head = next_head;
    
    for (int i = 0; i < ch.waiter_count; i++) {
        TaskScheduler::unblock(ch.waiting_tids[i]);
    }
    ch.waiter_count = 0;
    
    return true;
}

uint32_t EventSystem::pop(int channel_id) {
    InterruptGuard guard;
    
    if (channel_id < 0 || channel_id >= MAX_CHANNELS) return 0;
    EventChannel& ch = channels_[channel_id];
    if (!ch.active) return 0;
    
    if (ch.head == ch.tail) {
        return 0;
    }
    
    uint32_t data = ch.buffer[ch.tail];
    ch.tail = (ch.tail + 1) % CHANNEL_BUFFER_SIZE;
    return data;
}

uint32_t EventSystem::wait(int channel_id) {
    if (channel_id < 0 || channel_id >= MAX_CHANNELS) return 0;
    
    while (true) {
        {
            InterruptGuard guard;
            EventChannel& ch = channels_[channel_id];
            if (!ch.active) return 0;
            
            if (ch.head != ch.tail) {
                uint32_t data = ch.buffer[ch.tail];
                ch.tail = (ch.tail + 1) % CHANNEL_BUFFER_SIZE;
                return data;
            }
            
            if (ch.waiter_count < MAX_WAITERS) {
                ch.waiting_tids[ch.waiter_count++] = current_tid;
            }
        }
        
        TaskScheduler::block_current(channel_id);
    }
}

} // namespace re36
