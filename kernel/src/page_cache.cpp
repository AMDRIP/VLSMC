#include "kernel/page_cache.h"
#include "kernel/pmm.h"

namespace re36 {

PageCacheEntry PageCache::entries_[PAGE_CACHE_SIZE];

void PageCache::init() {
    for (int i = 0; i < PAGE_CACHE_SIZE; i++) {
        entries_[i].valid = false;
    }
}

uint32_t PageCache::hash(uint32_t inode, uint32_t offset) {
    return (inode * 2654435761u + offset / 4096) % PAGE_CACHE_SIZE;
}

uint32_t PageCache::lookup(uint32_t inode, uint32_t offset) {
    uint32_t h = hash(inode, offset);
    for (uint32_t i = 0; i < 4; i++) {
        uint32_t idx = (h + i) % PAGE_CACHE_SIZE;
        if (entries_[idx].valid &&
            entries_[idx].inode == inode &&
            entries_[idx].offset == offset) {
            return entries_[idx].phys_frame;
        }
    }
    return 0;
}

void PageCache::insert(uint32_t inode, uint32_t offset, uint32_t phys_frame) {
    uint32_t h = hash(inode, offset);

    for (uint32_t i = 0; i < 4; i++) {
        uint32_t idx = (h + i) % PAGE_CACHE_SIZE;
        if (!entries_[idx].valid) {
            entries_[idx].inode = inode;
            entries_[idx].offset = offset;
            entries_[idx].phys_frame = phys_frame;
            entries_[idx].valid = true;
            return;
        }
        if (entries_[idx].inode == inode && entries_[idx].offset == offset) {
            entries_[idx].phys_frame = phys_frame;
            return;
        }
    }

    uint32_t idx = h % PAGE_CACHE_SIZE;
    entries_[idx].inode = inode;
    entries_[idx].offset = offset;
    entries_[idx].phys_frame = phys_frame;
    entries_[idx].valid = true;
}

void PageCache::invalidate(uint32_t inode) {
    for (int i = 0; i < PAGE_CACHE_SIZE; i++) {
        if (entries_[i].valid && entries_[i].inode == inode) {
            entries_[i].valid = false;
        }
    }
}

} // namespace re36
