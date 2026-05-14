#include "kernel/pmm.h"
#include "kernel/spinlock.h"

namespace re36 {

uint32_t* PhysicalMemoryManager::memory_bitmap_ = nullptr;
uint32_t  PhysicalMemoryManager::max_frames_ = 0;
uint32_t  PhysicalMemoryManager::used_frames_ = 0;
uint32_t  PhysicalMemoryManager::usable_frames_ = 0;
uint32_t  PhysicalMemoryManager::direct_usable_frames_ = 0;
uint32_t  PhysicalMemoryManager::high_usable_frames_ = 0;
uint32_t  PhysicalMemoryManager::managed_memory_limit_ = 0;
uint32_t  PhysicalMemoryManager::direct_map_limit_ = 0;
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

void PhysicalMemoryManager::init(uint32_t bitmap_addr, uint32_t memory_size, uint32_t direct_map_limit) {
    memory_bitmap_ = (uint32_t*)bitmap_addr;
    max_frames_ = memory_size / PMM_FRAME_SIZE;
    managed_memory_limit_ = max_frames_ * PMM_FRAME_SIZE;
    direct_map_limit_ = direct_map_limit & ~(PMM_FRAME_SIZE - 1);

    used_frames_ = max_frames_;
    usable_frames_ = 0;
    direct_usable_frames_ = 0;
    high_usable_frames_ = 0;
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
    uint32_t total_memory = managed_memory_limit_;
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
            usable_frames_++;
            if (frame * PMM_FRAME_SIZE < direct_map_limit_) {
                direct_usable_frames_++;
            } else {
                high_usable_frames_++;
            }
        }
    }
}

void PhysicalMemoryManager::set_region_used(uint32_t base, uint32_t size) {
    InterruptGuard guard;

    uint64_t end_addr = (uint64_t)base + size;
    uint32_t total_memory = managed_memory_limit_;
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

uint32_t PhysicalMemoryManager::get_first_free_frame(uint32_t start_frame, uint32_t end_frame) {
    if (start_frame >= max_frames_) return 0xFFFFFFFF;
    if (end_frame > max_frames_) end_frame = max_frames_;
    if (end_frame <= start_frame) return 0xFFFFFFFF;

    uint32_t bitmap_words = get_bitmap_words(max_frames_);

    uint32_t start_word = PMM_BITMAP_INDEX(start_frame);
    uint32_t end_word = PMM_BITMAP_INDEX(end_frame - 1);

    for (uint32_t i = start_word; i <= end_word && i < bitmap_words; i++) {
        if (memory_bitmap_[i] == 0xFFFFFFFF) {
            continue;
        }

        for (uint32_t bit = 0; bit < kBitmapWordBits; bit++) {
            uint32_t frame = i * kBitmapWordBits + bit;
            if (frame < start_frame) {
                continue;
            }
            if (frame >= end_frame || frame >= max_frames_) {
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
    uint32_t end_frame = direct_map_limit_ / PMM_FRAME_SIZE;

    uint32_t frame = get_first_free_frame(0, end_frame);
    if (frame == 0xFFFFFFFF) return nullptr;

    set_frame(frame);
    used_frames_++;
    refcounts_[frame] = 1;

    return (void*)(frame * PMM_FRAME_SIZE);
}

void* PhysicalMemoryManager::alloc_high_frame() {
    InterruptGuard guard;
    uint32_t start_frame = direct_map_limit_ / PMM_FRAME_SIZE;

    uint32_t frame = get_first_free_frame(start_frame, max_frames_);
    if (frame == 0xFFFFFFFF) return nullptr;

    set_frame(frame);
    used_frames_++;
    refcounts_[frame] = 1;

    return (void*)(frame * PMM_FRAME_SIZE);
}

void* PhysicalMemoryManager::alloc_user_frame() {
    void* frame = alloc_high_frame();
    if (frame) return frame;
    return alloc_frame();
}

void* PhysicalMemoryManager::alloc_frame_any() {
    InterruptGuard guard;
    uint32_t frame = get_first_free_frame(0, max_frames_);
    if (frame == 0xFFFFFFFF) return nullptr;

    set_frame(frame);
    used_frames_++;
    refcounts_[frame] = 1;

    return (void*)(frame * PMM_FRAME_SIZE);
}

uint32_t PhysicalMemoryManager::get_free_blocks(uint32_t count, uint32_t start_range_frame, uint32_t end_frame) {
    if (count == 0) return 0xFFFFFFFF;
    if (end_frame > max_frames_) end_frame = max_frames_;
    if (end_frame <= start_range_frame) return 0xFFFFFFFF;
    if (count == 1) return get_first_free_frame(start_range_frame, end_frame);

    uint32_t current_count = 0;
    uint32_t candidate_start = 0xFFFFFFFF;

    for (uint32_t i = start_range_frame; i < end_frame; i++) {
        if (!test_frame(i)) {
            if (current_count == 0) candidate_start = i;
            current_count++;
            if (current_count == count) return candidate_start;
        } else {
            current_count = 0;
        }
    }
    return 0xFFFFFFFF;
}

void* PhysicalMemoryManager::alloc_blocks(uint32_t count) {
    InterruptGuard guard;
    if (count == 0) return nullptr;

    uint32_t direct_end_frame = direct_map_limit_ / PMM_FRAME_SIZE;
    if (direct_end_frame > max_frames_) direct_end_frame = max_frames_;

    uint32_t start_frame = get_free_blocks(count, 0, direct_end_frame);
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
    return managed_memory_limit_;
}

uint32_t PhysicalMemoryManager::get_managed_memory_limit() {
    return managed_memory_limit_;
}

uint32_t PhysicalMemoryManager::get_direct_map_limit() {
    return direct_map_limit_;
}

uint32_t PhysicalMemoryManager::get_direct_mapped_memory() {
    return direct_usable_frames_ * PMM_FRAME_SIZE;
}

uint32_t PhysicalMemoryManager::get_high_memory() {
    return high_usable_frames_ * PMM_FRAME_SIZE;
}

uint32_t PhysicalMemoryManager::count_free_frames(uint32_t start_frame, uint32_t end_frame) {
    if (start_frame >= max_frames_) return 0;
    if (end_frame > max_frames_) end_frame = max_frames_;
    if (end_frame <= start_frame) return 0;

    uint32_t count = 0;
    for (uint32_t frame = start_frame; frame < end_frame; frame++) {
        if (!test_frame(frame)) count++;
    }
    return count;
}

uint32_t PhysicalMemoryManager::get_free_direct_mapped_memory() {
    uint32_t end_frame = direct_map_limit_ / PMM_FRAME_SIZE;
    return count_free_frames(0, end_frame) * PMM_FRAME_SIZE;
}

uint32_t PhysicalMemoryManager::get_free_high_memory() {
    uint32_t start_frame = direct_map_limit_ / PMM_FRAME_SIZE;
    return count_free_frames(start_frame, max_frames_) * PMM_FRAME_SIZE;
}

bool PhysicalMemoryManager::is_direct_mapped(uint32_t phys_addr) {
    return phys_addr < direct_map_limit_;
}

} // namespace re36
