#include "kernel/pmm.h"
#include "kernel/spinlock.h"

namespace re36 {

uint32_t* PhysicalMemoryManager::memory_bitmap_ = nullptr;
uint32_t  PhysicalMemoryManager::max_frames_ = 0;
uint32_t  PhysicalMemoryManager::used_frames_ = 0;
uint8_t*  PhysicalMemoryManager::refcounts_ = nullptr;

namespace {

constexpr uint32_t kBitmapWordBits = 32;

uint32_t align_up(uint32_t value, uint32_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

uint32_t align_down(uint32_t value, uint32_t alignment) {
    return value & ~(alignment - 1);
}

uint32_t get_bitmap_words(uint32_t frame_count) {
    return (frame_count + (kBitmapWordBits - 1)) / kBitmapWordBits;
}

} // namespace

inline void PhysicalMemoryManager::set_frame(uint32_t frame) {
    memory_bitmap_[PMM_BITMAP_INDEX(frame)] |= (1u << PMM_BITMAP_OFFSET(frame));
}

inline void PhysicalMemoryManager::clear_frame(uint32_t frame) {
    memory_bitmap_[PMM_BITMAP_INDEX(frame)] &= ~(1u << PMM_BITMAP_OFFSET(frame));
}

inline bool PhysicalMemoryManager::test_frame(uint32_t frame) {
    return memory_bitmap_[PMM_BITMAP_INDEX(frame)] & (1u << PMM_BITMAP_OFFSET(frame));
}

uint32_t PhysicalMemoryManager::calculate_metadata_size(uint32_t memory_size) {
    uint32_t frame_count = memory_size / PMM_FRAME_SIZE;
    return get_bitmap_words(frame_count) * sizeof(uint32_t) + frame_count;
}

void PhysicalMemoryManager::init(uint32_t bitmap_addr, uint32_t memory_size) {
    memory_bitmap_ = (uint32_t*)bitmap_addr;
    max_frames_ = memory_size / PMM_FRAME_SIZE;

    used_frames_ = max_frames_;
    uint32_t bitmap_words = get_bitmap_words(max_frames_);

    for (uint32_t i = 0; i < bitmap_words; i++) {
        memory_bitmap_[i] = 0xFFFFFFFF;
    }

    uint32_t bitmap_bytes = bitmap_words * sizeof(uint32_t);
    refcounts_ = (uint8_t*)(bitmap_addr + bitmap_bytes);
    for (uint32_t i = 0; i < max_frames_; i++) {
        refcounts_[i] = 0;
    }
}

void PhysicalMemoryManager::set_region_free(uint32_t base, uint32_t size) {
    InterruptGuard guard;

    uint64_t end_addr = (uint64_t)base + size;
    uint32_t total_memory = get_total_memory();
    uint32_t start_addr = align_up(base, PMM_FRAME_SIZE);
    uint32_t end_aligned = align_down((end_addr > total_memory) ? total_memory : (uint32_t)end_addr,
                                      PMM_FRAME_SIZE);

    if (end_aligned <= start_addr) {
        return;
    }

    uint32_t frame = start_addr / PMM_FRAME_SIZE;
    uint32_t frame_count = (end_aligned - start_addr) / PMM_FRAME_SIZE;

    for (; frame_count > 0; frame_count--, frame++) {
        if (test_frame(frame)) {
            clear_frame(frame);
            used_frames_--;
        }
    }
}

void PhysicalMemoryManager::set_region_used(uint32_t base, uint32_t size) {
    InterruptGuard guard;

    uint64_t end_addr = (uint64_t)base + size;
    uint32_t total_memory = get_total_memory();
    uint32_t start_addr = align_down(base, PMM_FRAME_SIZE);
    uint32_t end_aligned = align_up((end_addr > total_memory) ? total_memory : (uint32_t)end_addr,
                                    PMM_FRAME_SIZE);

    if (start_addr >= total_memory || end_aligned <= start_addr) {
        return;
    }

    uint32_t frame = start_addr / PMM_FRAME_SIZE;
    uint32_t frame_count = (end_aligned - start_addr) / PMM_FRAME_SIZE;

    for (; frame_count > 0; frame_count--, frame++) {
        if (!test_frame(frame)) {
            set_frame(frame);
            used_frames_++;
        }
    }
}

uint32_t PhysicalMemoryManager::get_first_free_frame() {
    uint32_t bitmap_words = get_bitmap_words(max_frames_);

    for (uint32_t i = 0; i < bitmap_words; i++) {
        if (memory_bitmap_[i] == 0xFFFFFFFF) {
            continue;
        }

        for (uint32_t bit = 0; bit < kBitmapWordBits; bit++) {
            uint32_t frame = i * kBitmapWordBits + bit;
            if (frame >= max_frames_) {
                break;
            }

            if (!(memory_bitmap_[i] & (1u << bit))) {
                return frame;
            }
        }
    }

    return 0xFFFFFFFF;
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
    return 0xFFFFFFFF;
}

void* PhysicalMemoryManager::alloc_blocks(uint32_t count) {
    InterruptGuard guard;
    if (get_free_memory() < count * PMM_FRAME_SIZE) return nullptr;

    uint32_t start_frame = get_free_blocks(count);
    if (start_frame == 0xFFFFFFFF) return nullptr;

    for (uint32_t i = 0; i < count; i++) {
        set_frame(start_frame + i);
        used_frames_++;
        refcounts_[start_frame + i] = 1;
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

uint32_t PhysicalMemoryManager::get_total_memory() {
    return max_frames_ * PMM_FRAME_SIZE;
}

} // namespace re36
