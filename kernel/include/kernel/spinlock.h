#pragma once

#include <stdint.h>

namespace re36 {

inline uint32_t cli_save() {
    uint32_t flags;
    asm volatile("pushf; pop %0; cli" : "=r"(flags));
    return flags;
}

inline void sti_restore(uint32_t flags) {
    asm volatile("push %0; popf" :: "r"(flags));
}

struct InterruptGuard {
    uint32_t saved_flags;
    
    InterruptGuard() {
        saved_flags = cli_save();
    }
    
    ~InterruptGuard() {
        sti_restore(saved_flags);
    }
};

} // namespace re36
