#include "kernel/pmm.h"
#include "kernel/spinlock.h"

namespace re36 {

uint32_t* PhysicalMemoryManager::memory_bitmap_ = nullptr;
uint32_t  PhysicalMemoryManager::max_frames_ = 0;
uint32_t  PhysicalMemoryManager::used_frames_ = 0;
uint8_t*  PhysicalMemoryManager::refcounts_ = nullptr;

inline void PhysicalMemoryManager::set_frame(uint32_t frame) {
    memory_bitmap_[PMM_BITMAP_INDEX(frame)] |= (1 << PMM_BITMAP_OFFSET(frame));
}

inline void PhysicalMemoryManager::clear_frame(uint32_t frame) {
    memory_bitmap_[PMM_BITMAP_INDEX(frame)] &= ~(1 << PMM_BITMAP_OFFSET(frame));
}

inline bool PhysicalMemoryManager::test_frame(uint32_t frame) {
    return memory_bitmap_[PMM_BITMAP_INDEX(frame)] & (1 << PMM_BITMAP_OFFSET(frame));
}

void PhysicalMemoryManager::init(uint32_t bitmap_addr, uint32_t memory_size) {
    memory_bitmap_ = (uint32_t*)bitmap_addr;
    max_frames_ = memory_size / PMM_FRAME_SIZE;
    
    used_frames_ = max_frames_;
    uint32_t bitmap_size = max_frames_ / 32;
    
    for (uint32_t i = 0; i < bitmap_size; i++) {
        memory_bitmap_[i] = 0xFFFFFFFF;
    }

    uint32_t bitmap_bytes = (max_frames_ + 31) / 32 * 4;
    refcounts_ = (uint8_t*)(bitmap_addr + bitmap_bytes);
    for (uint32_t i = 0; i < max_frames_; i++) {
        refcounts_[i] = 0;
    }
}

void PhysicalMemoryManager::set_region_free(uint32_t base, uint32_t size) {
    InterruptGuard guard;
    uint32_t align = base / PMM_FRAME_SIZE;
    uint32_t frames = size / PMM_FRAME_SIZE;

    for (; frames > 0; frames--, align++) {
        if (test_frame(align)) {
            clear_frame(align);
            used_frames_--;
        }
    }
}

void PhysicalMemoryManager::set_region_used(uint32_t base, uint32_t size) {
    InterruptGuard guard;
    uint32_t align = base / PMM_FRAME_SIZE;
    uint32_t frames = size / PMM_FRAME_SIZE;

    for (; frames > 0; frames--, align++) {
        if (!test_frame(align)) {
            set_frame(align);
            used_frames_++;
        }
    }
}

uint32_t PhysicalMemoryManager::get_first_free_frame() {
    for (uint32_t i = 0; i < PMM_BITMAP_INDEX(max_frames_); i++) {
        // Если блок (32 фрейма) не полностью занят
        if (memory_bitmap_[i] != 0xFFFFFFFF) {
            for (int j = 0; j < 32; j++) {
                uint32_t bit = 1 << j;
                if (!(memory_bitmap_[i] & bit)) { // Если бит = 0 (свободен)
                    return i * 32 + j; // Возвращаем индекс фрейма
                }
            }
        }
    }
    return 0xFFFFFFFF; // Нет памяти
}

void* PhysicalMemoryManager::alloc_frame() {
    InterruptGuard guard;
    if (get_free_memory() == 0) return nullptr;

    uint32_t frame = get_first_free_frame();
    if (frame == 0xFFFFFFFF) return nullptr;

    set_frame(frame);
    used_frames_++;
    refcounts_[frame] = 1;
    
    return (void*)(frame * PMM_FRAME_SIZE);
}

uint32_t PhysicalMemoryManager::get_free_blocks(uint32_t count) {
    if (count == 0) return 0xFFFFFFFF;
    if (count == 1) return get_first_free_frame();

    uint32_t current_count = 0;
    uint32_t start_frame = 0xFFFFFFFF;

    for (uint32_t i = 0; i < max_frames_; i++) {
        if (!test_frame(i)) {
            if (current_count == 0) start_frame = i;
            current_count++;
            if (current_count == count) return start_frame;
        } else {
            current_count = 0;
        }
    }
    return 0xFFFFFFFF; // Нет подходящего блока
}

void* PhysicalMemoryManager::alloc_blocks(uint32_t count) {
    InterruptGuard guard;
    if (get_free_memory() < count * PMM_FRAME_SIZE) return nullptr;

    uint32_t start_frame = get_free_blocks(count);
    if (start_frame == 0xFFFFFFFF) return nullptr;

    for (uint32_t i = 0; i < count; i++) {
        set_frame(start_frame + i);
        used_frames_++;
    }
    
    return (void*)(start_frame * PMM_FRAME_SIZE);
}

void PhysicalMemoryManager::free_frame(void* frame_addr) {
    InterruptGuard guard;
    uint32_t addr = (uint32_t)frame_addr;
    uint32_t frame = addr / PMM_FRAME_SIZE;
    if (frame >= max_frames_) return;

    if (refcounts_[frame] > 1) {
        refcounts_[frame]--;
        return;
    }

    refcounts_[frame] = 0;
    if (test_frame(frame)) {
        clear_frame(frame);
        used_frames_--;
    }
}

void PhysicalMemoryManager::inc_ref(uint32_t phys_addr) {
    InterruptGuard guard;
    uint32_t frame = phys_addr / PMM_FRAME_SIZE;
    if (frame < max_frames_ && refcounts_[frame] < 255) {
        refcounts_[frame]++;
    }
}

void PhysicalMemoryManager::dec_ref(uint32_t phys_addr) {
    InterruptGuard guard;
    uint32_t frame = phys_addr / PMM_FRAME_SIZE;
    if (frame >= max_frames_) return;
    free_frame((void*)phys_addr);
}

uint8_t PhysicalMemoryManager::get_refcount(uint32_t phys_addr) {
    uint32_t frame = phys_addr / PMM_FRAME_SIZE;
    if (frame >= max_frames_) return 0;
    return refcounts_[frame];
}

uint32_t PhysicalMemoryManager::get_free_memory() {
    return (max_frames_ - used_frames_) * PMM_FRAME_SIZE;
}

uint32_t PhysicalMemoryManager::get_used_memory() {
    return used_frames_ * PMM_FRAME_SIZE;
}

} // namespace re36
