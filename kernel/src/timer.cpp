#include "kernel/timer.h"
#include "kernel/pic.h"
#include "kernel/task_scheduler.h"

namespace re36 {

uint32_t Timer::ticks_ = 0;
uint32_t Timer::frequency_ = 100;

void Timer::init(uint32_t frequency_hz) {
    frequency_ = frequency_hz;
    ticks_ = 0;

    uint32_t divisor = 1193180 / frequency_hz;

    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}

void Timer::tick() {
    ticks_++;
}

uint32_t Timer::get_ticks() {
    return ticks_;
}

void Timer::sleep(uint32_t ms) {
    uint32_t ticks_to_wait = (ms * frequency_) / 1000;
    if (ticks_to_wait == 0) ticks_to_wait = 1;
    TaskScheduler::sleep_current(ms);
}

} // namespace re36
