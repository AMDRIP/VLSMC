#pragma once

#include <stdint.h>

namespace re36 {

struct vnode;

#define PAGE_CACHE_SIZE 256

struct PageCacheEntry {
    uint32_t inode;
    uint32_t offset;
    uint32_t phys_frame;
    bool     valid;
};

class PageCache {
public:
    static void init();
    static uint32_t lookup(uint32_t inode, uint32_t offset);
    static void insert(uint32_t inode, uint32_t offset, uint32_t phys_frame);
    static void invalidate(uint32_t inode);

private:
    static PageCacheEntry entries_[PAGE_CACHE_SIZE];
    static uint32_t hash(uint32_t inode, uint32_t offset);
};

} // namespace re36
