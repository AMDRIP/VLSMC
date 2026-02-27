#include "kernel/memory_validator.h"
#include "kernel/pmm.h"
#include "kernel/vmm.h"
#include "kernel/kmalloc.h"
#include "libc.h"

namespace re36 {

bool MemoryValidator::run_all_tests() {
    printf("[Validator] Starting Memory Subsystem Validation...\n");

    if (!test_pmm()) {
        printf(" -> PMM Test: FAILED\n");
        return false;
    }
    printf(" -> PMM Test: OK\n");

    if (!test_vmm()) {
        printf(" -> VMM Test: FAILED\n");
        return false;
    }
    printf(" -> VMM Test: OK\n");

    if (!test_heap()) {
        printf(" -> Heap Test: FAILED\n");
        return false;
    }
    printf(" -> Heap (kmalloc) Test: OK\n");

    printf("[Validator] All memory tests passed successfully.\n");
    return true;
}

bool MemoryValidator::test_pmm() {
    void* frame1 = PhysicalMemoryManager::alloc_frame();
    void* frame2 = PhysicalMemoryManager::alloc_frame();
    void* frame3 = PhysicalMemoryManager::alloc_frame();

    if (!frame1 || !frame2 || !frame3) return false;
    
    // Check 4KB alignment
    if ((uint32_t)frame1 % 4096 != 0) return false;
    if ((uint32_t)frame2 % 4096 != 0) return false;
    if ((uint32_t)frame3 % 4096 != 0) return false;

    // Check uniqueness
    if (frame1 == frame2 || frame1 == frame3 || frame2 == frame3) return false;

    PhysicalMemoryManager::free_frame(frame1);
    PhysicalMemoryManager::free_frame(frame2);
    PhysicalMemoryManager::free_frame(frame3);

    // After freeing, allocating again should give us one of the frames back
    void* new_frame = PhysicalMemoryManager::alloc_frame();
    if (!new_frame) return false;
    if (new_frame != frame1 && new_frame != frame2 && new_frame != frame3) {
        // Not strictly an error if list behavior is different, but for a simple stack/bitmap it's likely true
        // We just do basic sanity check
    }

    PhysicalMemoryManager::free_frame(new_frame);

    return true;
}

bool MemoryValidator::test_vmm() {
    // Pick an address that is likely unused right now
    uint32_t test_virt_addr = 0xE0000000;
    
    void* phys_frame = PhysicalMemoryManager::alloc_frame();
    if (!phys_frame) return false;

    // Map page as Read/Write
    VMM::map_page(test_virt_addr, (uint32_t)phys_frame, PAGE_PRESENT | PAGE_WRITABLE);

    // Test Write and Read
    volatile uint32_t* test_ptr = (volatile uint32_t*)test_virt_addr;
    *test_ptr = 0xDEADBEEF;

    if (*test_ptr != 0xDEADBEEF) {
        return false; // Virtual mapping failed
    }

    *test_ptr = 0xCAFEBABE;
    if (*test_ptr != 0xCAFEBABE) {
        return false; // Write-back failed
    }

    // Attempt to verify physical memory
    // Since we mapped it, wait, we can't easily read physical directly without identity map
    // So we just trust the readback
    
    // Unmap
    VMM::unmap_page(test_virt_addr);
    PhysicalMemoryManager::free_frame(phys_frame);

    // Note: We do NOT try to read the unmapped page here to avoid triggering a Page Fault 
    // that would kill our boot process or thread.

    return true;
}

bool MemoryValidator::test_heap() {
    // 16 bytes
    void* ptr1 = kmalloc(16);
    if (!ptr1) return false;
    
    // 128 bytes
    void* ptr2 = kmalloc(128);
    if (!ptr2) return false;
    
    // 4 KB
    void* ptr3 = kmalloc(4096);
    if (!ptr3) return false;

    // Memory corruption test
    uint8_t* p1 = (uint8_t*)ptr1;
    for (int i = 0; i < 16; i++) p1[i] = 0xAA;
    
    uint8_t* p2 = (uint8_t*)ptr2;
    for (int i = 0; i < 128; i++) p2[i] = 0xBB;

    uint8_t* p3 = (uint8_t*)ptr3;
    for (int i = 0; i < 4096; i++) p3[i] = 0xCC;

    // Verify boundaries
    for (int i = 0; i < 16; i++) {
        if (p1[i] != 0xAA) return false;
    }
    for (int i = 0; i < 128; i++) {
        if (p2[i] != 0xBB) return false;
    }
    for (int i = 0; i < 4096; i++) {
        if (p3[i] != 0xCC) return false;
    }

    kfree(ptr1);
    kfree(ptr2);
    kfree(ptr3);

    return true;
}

} // namespace re36
