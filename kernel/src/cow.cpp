#include "kernel/cow.h"
#include "kernel/vmm.h"
#include "kernel/pmm.h"
#include "kernel/spinlock.h"
#include "kernel/thread.h"
#include "kernel/task_scheduler.h"
#include "libc.h"

namespace re36 {

static inline void cow_invlpg(uint32_t addr) {
    asm volatile("invlpg (%0)" :: "r"(addr) : "memory");
}

static inline uint32_t* cow_get_pde_ptr(uint32_t pd_index) {
    return &((uint32_t*)PAGE_DIR_VADDR)[pd_index];
}

static inline uint32_t* cow_get_pte_ptr(uint32_t virt) {
    uint32_t index = virt >> 12;
    return &((uint32_t*)PAGE_TABLES_VADDR)[index];
}

uint32_t* cow_clone_directory() {
    InterruptGuard guard;

    uint32_t* new_dir = (uint32_t*)PhysicalMemoryManager::alloc_frame();
    if (!new_dir) return nullptr;

    for (int i = 0; i < PD_ENTRIES; i++) {
        new_dir[i] = 0;
    }

    uint32_t* cur_pd = (uint32_t*)PAGE_DIR_VADDR;

    for (int i = 0; i < PD_ENTRIES; i++) {
        if (i == RECURSIVE_PD_INDEX) continue;
        if (!(cur_pd[i] & PAGE_PRESENT)) continue;

        if (!(cur_pd[i] & PAGE_USER)) {
            new_dir[i] = cur_pd[i];
            continue;
        }

        uint32_t* new_pt = (uint32_t*)PhysicalMemoryManager::alloc_frame();
        if (!new_pt) {
            for (int k = 0; k < i; k++) {
                if (k == RECURSIVE_PD_INDEX) continue;
                if (!(new_dir[k] & PAGE_PRESENT)) continue;
                if (!(new_dir[k] & PAGE_USER)) continue;

                uint32_t* pt = (uint32_t*)(new_dir[k] & 0xFFFFF000);
                for (int j = 0; j < PT_ENTRIES; j++) {
                    if (pt[j] & PAGE_PRESENT) {
                        uint32_t phys = pt[j] & 0xFFFFF000;
                        PhysicalMemoryManager::dec_ref(phys);

                        if (pt[j] & PAGE_COW) {
                            uint32_t* src_pte = cow_get_pte_ptr((k << 22) | (j << 12));
                            *src_pte = phys | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
                            *src_pte &= ~PAGE_COW;
                            cow_invlpg((k << 22) | (j << 12));
                        }
                    }
                }
                PhysicalMemoryManager::free_frame((void*)pt);
            }
            PhysicalMemoryManager::free_frame((void*)new_dir);
            return nullptr;
        }

        for (int j = 0; j < PT_ENTRIES; j++) {
            uint32_t* src_pte = cow_get_pte_ptr((i << 22) | (j << 12));

            if (!(*src_pte & PAGE_PRESENT)) {
                new_pt[j] = 0;
                continue;
            }

            uint32_t phys = *src_pte & 0xFFFFF000;
            uint32_t flags = *src_pte & 0xFFF;

            if (flags & PAGE_WRITABLE) {
                new_pt[j] = phys | PAGE_PRESENT | PAGE_USER | PAGE_COW;
                new_pt[j] &= ~PAGE_WRITABLE;

                *src_pte = phys | PAGE_PRESENT | PAGE_USER | PAGE_COW;
                *src_pte &= ~PAGE_WRITABLE;
            } else {
                new_pt[j] = *src_pte;
            }

            PhysicalMemoryManager::inc_ref(phys);

            uint32_t virt_addr = (i << 22) | (j << 12);
            cow_invlpg(virt_addr);
        }

        new_dir[i] = (uint32_t)new_pt | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
    }

    new_dir[RECURSIVE_PD_INDEX] = (uint32_t)new_dir | PAGE_PRESENT | PAGE_WRITABLE;

    return new_dir;
}

bool cow_handle_fault(uint32_t fault_addr, uint32_t error_code) {
    bool is_present = (error_code & 0x1) != 0;
    bool is_write   = (error_code & 0x2) != 0;

    if (!is_present || !is_write) return false;

    uint32_t pd_index = fault_addr >> 22;
    uint32_t* pde = cow_get_pde_ptr(pd_index);
    if (!(*pde & PAGE_PRESENT)) return false;

    uint32_t* pte = cow_get_pte_ptr(fault_addr);
    uint32_t pte_val = *pte;

    if (!(pte_val & PAGE_COW)) return false;

    uint32_t old_phys = pte_val & 0xFFFFF000;

    if (PhysicalMemoryManager::get_refcount(old_phys) == 1) {
        *pte = old_phys | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
        *pte &= ~PAGE_COW;
        cow_invlpg(fault_addr & 0xFFFFF000);
        return true;
    }

    void* new_frame = PhysicalMemoryManager::alloc_frame();
    if (!new_frame) {
        printf("\n[CoW] OOM at 0x%x — killing TID %d\n", fault_addr, current_tid);
        if (current_tid > 0) {
            thread_terminate(current_tid);
        }
        return false;
    }

    uint8_t* src = (uint8_t*)(old_phys);
    uint8_t* dst = (uint8_t*)new_frame;
    for (int i = 0; i < (int)PAGE_SIZE; i++) {
        dst[i] = src[i];
    }

    PhysicalMemoryManager::dec_ref(old_phys);

    *pte = (uint32_t)new_frame | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
    *pte &= ~PAGE_COW;

    cow_invlpg(fault_addr & 0xFFFFF000);
    return true;
}

} // namespace re36
