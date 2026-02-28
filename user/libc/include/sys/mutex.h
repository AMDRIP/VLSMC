#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef volatile int mutex_t;

// simple spinlock using GCC atomic builtin
static inline void mutex_lock(mutex_t* m) {
    while (__atomic_test_and_set(m, __ATOMIC_ACQUIRE)) {
        // Spin. In a real OS with futexes, we'd yield or sleep here after some spins.
        // For VLSMC, we just busy-wait (or we could invoke sys_yield eventually)
        __asm__ volatile("pause" ::: "memory");
    }
}

static inline void mutex_unlock(mutex_t* m) {
    __atomic_clear(m, __ATOMIC_RELEASE);
}

#ifdef __cplusplus
}
#endif
