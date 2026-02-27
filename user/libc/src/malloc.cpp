#include "malloc.h"
#include "string.h"
#include <sys/syscall.h>

#define ALIGNMENT 16
#define ALIGN(size) (((size) + ALIGNMENT - 1) & ~(ALIGNMENT - 1))

#define FLAG_FREE 1
#define GET_SIZE(b) ((b)->size & ~3)
#define IS_FREE(b) ((b)->size & FLAG_FREE)

struct Block {
    size_t prev_size;
    size_t size;
    Block* next;
    Block* prev;
};

static Block* free_list = nullptr;
static Block* fast_bins[4] = {nullptr};
static Block* last_physical = nullptr;

static void list_remove(Block* b) {
    if (b->prev) b->prev->next = b->next;
    else free_list = b->next;
    if (b->next) b->next->prev = b->prev;
}

static void list_add(Block* b) {
    b->prev = nullptr;
    b->next = free_list;
    if (free_list) free_list->prev = b;
    free_list = b;
}

static bool extend_heap(size_t min_size) {
    size_t fetch = ALIGN(min_size > 4096 ? min_size : 4096);
    char* mem = (char*)syscall(SYS_SBRK, fetch);
    if (mem == (char*)-1) return false;

    if (last_physical && ((char*)last_physical + GET_SIZE(last_physical) == mem)) {
        if (IS_FREE(last_physical)) {
            last_physical->size += fetch;
            return true;
        } else {
            Block* b = (Block*)mem;
            b->prev_size = GET_SIZE(last_physical);
            b->size = fetch | FLAG_FREE;
            b->next = nullptr;
            b->prev = nullptr;
            list_add(b);
            last_physical = b;
            return true;
        }
    }

    Block* b = (Block*)mem;
    b->prev_size = 0;
    b->size = fetch | FLAG_FREE;
    b->next = nullptr;
    b->prev = nullptr;
    list_add(b);
    last_physical = b;
    return true;
}

extern "C" void* malloc(size_t size) {
    if (size == 0) return nullptr;
    size_t total_size = ALIGN(size + 16);
    
    if (total_size <= 80) {
        int idx = (total_size - 32) / 16;
        if (fast_bins[idx]) {
            Block* b = fast_bins[idx];
            fast_bins[idx] = b->next;
            return (char*)b + 16;
        }
    }
    
retry:
    Block* curr = free_list;
    while (curr) {
        size_t c_size = GET_SIZE(curr);
        if (c_size >= total_size) {
            if (c_size >= total_size + 32) {
                list_remove(curr);
                
                Block* remainder = (Block*)((char*)curr + total_size);
                remainder->prev_size = total_size;
                remainder->size = (c_size - total_size) | FLAG_FREE;
                
                curr->size = total_size;
                
                if (curr != last_physical) {
                    Block* next_phys = (Block*)((char*)remainder + GET_SIZE(remainder));
                    next_phys->prev_size = GET_SIZE(remainder);
                } else {
                    last_physical = remainder;
                }
                
                list_add(remainder);
                return (char*)curr + 16;
            } else {
                list_remove(curr);
                curr->size &= ~FLAG_FREE;
                return (char*)curr + 16;
            }
        }
        curr = curr->next;
    }
    
    if (extend_heap(total_size)) {
        goto retry;
    }
    
    return nullptr;
}

extern "C" void free(void* ptr) {
    if (!ptr) return;
    Block* b = (Block*)((char*)ptr - 16);
    size_t b_size = GET_SIZE(b);
    
    if (b_size <= 80) {
        int idx = (b_size - 32) / 16;
        b->next = fast_bins[idx];
        fast_bins[idx] = b;
        return;
    }
    
    Block* prev_phys = b->prev_size ? (Block*)((char*)b - b->prev_size) : nullptr;
    Block* next_phys = (b != last_physical) ? (Block*)((char*)b + b_size) : nullptr;
    
    bool merge_prev = prev_phys && IS_FREE(prev_phys);
    bool merge_next = next_phys && IS_FREE(next_phys);
    
    if (merge_prev && merge_next) {
        list_remove(next_phys);
        prev_phys->size = (GET_SIZE(prev_phys) + b_size + GET_SIZE(next_phys)) | FLAG_FREE;
        
        if (next_phys == last_physical) {
            last_physical = prev_phys;
        } else {
            Block* after_next = (Block*)((char*)prev_phys + GET_SIZE(prev_phys));
            after_next->prev_size = GET_SIZE(prev_phys);
        }
    } else if (merge_prev) {
        prev_phys->size = (GET_SIZE(prev_phys) + b_size) | FLAG_FREE;
        if (b == last_physical) {
            last_physical = prev_phys;
        } else {
            next_phys->prev_size = GET_SIZE(prev_phys);
        }
    } else if (merge_next) {
        list_remove(next_phys);
        b->size = (b_size + GET_SIZE(next_phys)) | FLAG_FREE;
        list_add(b);
        if (next_phys == last_physical) {
            last_physical = b;
        } else {
            Block* after_next = (Block*)((char*)b + GET_SIZE(b));
            after_next->prev_size = GET_SIZE(b);
        }
    } else {
        b->size |= FLAG_FREE;
        list_add(b);
    }
}

extern "C" void* calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    void* ptr = malloc(total);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

extern "C" void* realloc(void* ptr, size_t size) {
    if (!ptr) return malloc(size);
    if (size == 0) {
        free(ptr);
        return nullptr;
    }
    
    Block* block = (Block*)((char*)ptr - 16);
    size_t b_size = GET_SIZE(block);
    size_t req_total = ALIGN(size + 16);
    
    if (b_size >= req_total) {
        return ptr;
    }
    
    void* new_ptr = malloc(size);
    if (!new_ptr) return nullptr;
    
    memcpy(new_ptr, ptr, b_size - 16);
    free(ptr);
    
    return new_ptr;
}
