#pragma once

#include <stdint.h>

namespace re36 {

class Timer {
public:
    static void init(uint32_t frequency_hz);
    
    static void tick();
    
    static uint32_t get_ticks();
    
    static void sleep(uint32_t ms);

private:
    static uint32_t ticks_;
    static uint32_t frequency_;
};

} // namespace re36
