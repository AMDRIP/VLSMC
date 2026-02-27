#include "malloc.h"
#include "string.h"
#include <sys/syscall.h>

#define ALIGNMENT 16
#define ALIGN(size) (((size) + ALIGNMENT - 1) & ~(ALIGNMENT - 1))

struct Block {
    size_t size;
    bool free;
    Block* next;
    Block* prev;
};

static Block* heap_start = nullptr;

extern "C" void* malloc(size_t size) {
    if (size == 0) return nullptr;
    
    size_t aligned_size = ALIGN(size + sizeof(Block));
    
    Block* current = heap_start;
    while (current) {
        if (current->free && current->size >= aligned_size) {
            if (current->size > aligned_size + sizeof(Block) + ALIGNMENT) {
                Block* new_block = (Block*)((char*)current + aligned_size);
                new_block->size = current->size - aligned_size;
                new_block->free = true;
                new_block->next = current->next;
                new_block->prev = current;
                
                if (current->next) current->next->prev = new_block;
                current->next = new_block;
                current->size = aligned_size;
            }
            current->free = false;
            return (void*)((char*)current + sizeof(Block));
        }
        current = current->next;
    }
    
    void* mem = (void*)syscall(SYS_SBRK, aligned_size);
    if (mem == (void*)-1) {
        return nullptr;
    }
    
    Block* new_block = (Block*)mem;
    new_block->size = aligned_size;
    new_block->free = false;
    new_block->next = nullptr;
    new_block->prev = nullptr;
    
    if (!heap_start) {
        heap_start = new_block;
    } else {
        Block* last = heap_start;
        while (last->next) last = last->next;
        last->next = new_block;
        new_block->prev = last;
    }
    
    return (void*)((char*)new_block + sizeof(Block));
}

extern "C" void free(void* ptr) {
    if (!ptr) return;
    
    Block* block = (Block*)((char*)ptr - sizeof(Block));
    block->free = true;
    
    if (block->prev && block->prev->free) {
        block->prev->size += block->size;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
        block = block->prev;
    }
    
    if (block->next && block->next->free) {
        block->size += block->next->size;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
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
    
    Block* block = (Block*)((char*)ptr - sizeof(Block));
    size_t aligned_size = ALIGN(size + sizeof(Block));
    
    if (block->size >= aligned_size) {
        return ptr;
    }
    
    void* new_ptr = malloc(size);
    if (!new_ptr) return nullptr;
    
    memcpy(new_ptr, ptr, block->size - sizeof(Block));
    free(ptr);
    
    return new_ptr;
}
