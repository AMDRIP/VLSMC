#include "kernel/vmm.h"
#include "kernel/pmm.h"
#include "kernel/spinlock.h"
#include "kernel/thread.h"
#include "kernel/task_scheduler.h"
#include "kernel/cow.h"
#include "kernel/vfs.h"
#include "libc.h"

namespace re36 {

uint32_t VMM::current_directory_phys_ = 0;
uint32_t VMM::kernel_directory_phys_ = 0;

static inline void invlpg(uint32_t addr) {
    asm volatile("invlpg (%0)" :: "r"(addr) : "memory");
}

static inline void load_cr3(uint32_t phys_addr) {
    asm volatile("mov %0, %%cr3" :: "r"(phys_addr) : "memory");
}

static inline uint32_t read_cr3() {
    uint32_t val;
    asm volatile("mov %%cr3, %0" : "=r"(val));
    return val;
}

static inline void enable_paging() {
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    asm volatile("mov %0, %%cr0" :: "r"(cr0) : "memory");
}

static inline uint32_t read_cr2() {
    uint32_t val;
    asm volatile("mov %%cr2, %0" : "=r"(val));
    return val;
}

static inline uint32_t* get_pde_ptr(uint32_t pd_index) {
    return &((uint32_t*)PAGE_DIR_VADDR)[pd_index];
}

static inline uint32_t* get_pte_ptr(uint32_t virt) {
    uint32_t index = virt >> 12;
    return &((uint32_t*)PAGE_TABLES_VADDR)[index];
}

void VMM::init() {
    uint32_t* page_dir = (uint32_t*)PhysicalMemoryManager::alloc_frame();
    if (!page_dir) {
        printf("FATAL: VMM cannot allocate Page Directory!\n");
        while(1) asm volatile("hlt");
    }

    for (int i = 0; i < PD_ENTRIES; i++) {
        page_dir[i] = 0;
    }

    for (uint32_t addr = 0; addr < KERNEL_SPACE_END; addr += PAGE_SIZE) {
        uint32_t pd_index = addr >> 22;
        uint32_t pt_index = (addr >> 12) & 0x3FF;

        if (!(page_dir[pd_index] & PAGE_PRESENT)) {
            uint32_t* new_table = (uint32_t*)PhysicalMemoryManager::alloc_frame();
            if (!new_table) {
                printf("FATAL: VMM cannot allocate Page Table!\n");
                while(1) asm volatile("hlt");
            }
            for (int j = 0; j < PT_ENTRIES; j++) {
                new_table[j] = 0;
            }
            page_dir[pd_index] = (uint32_t)new_table | PAGE_PRESENT | PAGE_WRITABLE;
        }

        uint32_t* pt = (uint32_t*)(page_dir[pd_index] & 0xFFFFF000);
        pt[pt_index] = addr | PAGE_PRESENT | PAGE_WRITABLE;
    }

    page_dir[RECURSIVE_PD_INDEX] = (uint32_t)page_dir | PAGE_PRESENT | PAGE_WRITABLE;

    current_directory_phys_ = (uint32_t)page_dir;
    kernel_directory_phys_ = current_directory_phys_;

    load_cr3(current_directory_phys_);
    enable_paging();
}

void VMM::map_page(uint32_t virt, uint32_t phys, uint32_t flags) {
    InterruptGuard guard;

    uint32_t pd_index = virt >> 22;
    uint32_t* pde = get_pde_ptr(pd_index);

    if (!(*pde & PAGE_PRESENT)) {
        uint32_t* new_table = (uint32_t*)PhysicalMemoryManager::alloc_frame();
        if (!new_table) return;

        uint32_t pd_flags = PAGE_PRESENT | PAGE_WRITABLE;
        if (flags & PAGE_USER) pd_flags |= PAGE_USER;
        *pde = (uint32_t)new_table | pd_flags;

        invlpg((uint32_t)get_pte_ptr(pd_index << 22));

        uint32_t* pt_base = get_pte_ptr(pd_index << 22);
        for (int i = 0; i < PT_ENTRIES; i++) {
            pt_base[i] = 0;
        }
    } else if ((flags & PAGE_USER) && !(*pde & PAGE_USER)) {
        *pde |= PAGE_USER;
    }

    uint32_t* pte = get_pte_ptr(virt);
    *pte = (phys & 0xFFFFF000) | (flags & 0xFFF) | PAGE_PRESENT;

    invlpg(virt);
}

void VMM::unmap_page(uint32_t virt) {
    InterruptGuard guard;

    uint32_t pd_index = virt >> 22;
    uint32_t* pde = get_pde_ptr(pd_index);

    if (!(*pde & PAGE_PRESENT)) return;

    uint32_t* pte = get_pte_ptr(virt);
    *pte = 0;

    invlpg(virt);
}

uint32_t VMM::get_physical(uint32_t virt) {
    uint32_t pd_index = virt >> 22;
    uint32_t* pde = get_pde_ptr(pd_index);

    if (!(*pde & PAGE_PRESENT)) return 0;

    uint32_t* pte = get_pte_ptr(virt);
    if (!(*pte & PAGE_PRESENT)) return 0;

    return (*pte & 0xFFFFF000) | (virt & 0xFFF);
}

void VMM::invalidate_page(uint32_t virt) {
    invlpg(virt);
}

void VMM::flush_tlb() {
    load_cr3(current_directory_phys_);
}

uint32_t* VMM::create_address_space() {
    InterruptGuard guard;

    uint32_t* new_dir = (uint32_t*)PhysicalMemoryManager::alloc_frame();
    if (!new_dir) return nullptr;

    for (int i = 0; i < PD_ENTRIES; i++) {
        new_dir[i] = 0;
    }

    uint32_t* cur_pd = (uint32_t*)PAGE_DIR_VADDR;
    for (uint32_t i = 0; i < PD_ENTRIES; i++) {
        if (i == RECURSIVE_PD_INDEX) continue;
        if ((cur_pd[i] & PAGE_PRESENT) && !(cur_pd[i] & PAGE_USER)) {
            new_dir[i] = cur_pd[i];
        }
    }

    new_dir[RECURSIVE_PD_INDEX] = (uint32_t)new_dir | PAGE_PRESENT | PAGE_WRITABLE;

    return new_dir;
}

void VMM::destroy_address_space(uint32_t* page_dir_phys) {
    if (!page_dir_phys) return;
    if (page_dir_phys == (uint32_t*)kernel_directory_phys_) return;

    InterruptGuard guard;

    for (int i = 0; i < PD_ENTRIES; i++) {
        if (i == RECURSIVE_PD_INDEX) continue;
        if (!(page_dir_phys[i] & PAGE_PRESENT)) continue;
        if (!(page_dir_phys[i] & PAGE_USER)) continue;

        uint32_t* pt = (uint32_t*)(page_dir_phys[i] & 0xFFFFF000);
        for (int j = 0; j < PT_ENTRIES; j++) {
            if (pt[j] & PAGE_PRESENT) {
                uint32_t phys_frame = pt[j] & 0xFFFFF000;
                PhysicalMemoryManager::dec_ref(phys_frame);
            }
        }
        PhysicalMemoryManager::free_frame((void*)pt);
    }

    PhysicalMemoryManager::free_frame((void*)page_dir_phys);
}

void VMM::switch_address_space(uint32_t* page_dir_phys) {
    current_directory_phys_ = (uint32_t)page_dir_phys;
    load_cr3(current_directory_phys_);
}

uint32_t* VMM::clone_directory() {
    return cow_clone_directory();
}

bool VMM::handle_page_fault(uint32_t fault_addr, uint32_t error_code) {
    uint32_t pd_index = fault_addr >> 22;
    uint32_t pt_index = (fault_addr >> 12) & 0x3FF;

    bool is_present = (error_code & 0x1) != 0;
    bool is_write   = (error_code & 0x2) != 0;
    bool is_user    = (error_code & 0x4) != 0;

    if (is_present && is_write) {
        if (cow_handle_fault(fault_addr, error_code)) return true;
    } else if (!is_present && is_user) {
        if (current_tid >= 0 && current_tid < MAX_THREADS) {
            Thread& cur = threads[current_tid];

            while (__atomic_test_and_set(&cur.heap_lock, __ATOMIC_ACQUIRE)) {
                asm volatile("pause");
            }
            uint32_t start = cur.heap_start;
            uint32_t end   = cur.heap_end;
            __atomic_clear(&cur.heap_lock, __ATOMIC_RELEASE);

            if (fault_addr >= start && fault_addr < end) {
                void* new_frame = PhysicalMemoryManager::alloc_frame();
                if (!new_frame) {
                    printf("\n!!! PAGE FAULT: Out of memory for heap at 0x%x !!!\n", fault_addr);
                    while(1) asm volatile("cli; hlt");
                }

                uint32_t page_addr = fault_addr & 0xFFFFF000;
                VMM::map_page(page_addr, (uint32_t)new_frame, PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);
                return true;
            }

            VMA* curr_vma = cur.vma_list;
            while (curr_vma) {
                if (fault_addr >= curr_vma->start && fault_addr < curr_vma->end) {
                    void* new_frame = PhysicalMemoryManager::alloc_frame();
                    if (!new_frame) {
                        printf("\n!!! PAGE FAULT: Out of memory for Demand Paging at 0x%x !!!\n", fault_addr);
                        while(1) asm volatile("cli; hlt");
                    }

                    uint32_t page_addr = fault_addr & ~0xFFF;
                    uint8_t* frame_ptr = (uint8_t*)new_frame;
                    for (int b = 0; b < 4096; b++) frame_ptr[b] = 0;

                    uint32_t file_data_end = curr_vma->start + curr_vma->file_size;

                    if (page_addr < file_data_end) {
                        uint32_t offset_in_vma = page_addr - curr_vma->start;
                        uint32_t file_offset = curr_vma->file_offset + offset_in_vma;

                        uint32_t read_size = 4096;
                        if (page_addr + 4096 > file_data_end) {
                            read_size = file_data_end - page_addr;
                        }

                        if (read_size > 0) {
                            if (curr_vma->type == VMA_TYPE_FILE && curr_vma->file_vnode) {
                                vnode* fvn = curr_vma->file_vnode;
                                if (fvn->ops && fvn->ops->read) {
                                    fvn->ops->read(fvn, file_offset, frame_ptr, read_size);
                                }
                            } else {
                                vnode* vn = nullptr;
                                if (vfs_resolve_path(cur.name, &vn) == 0 && vn && vn->ops && vn->ops->read) {
                                    vn->ops->read(vn, file_offset, frame_ptr, read_size);
                                    vnode_release(vn);
                                }
                            }
                        }
                    }

                    VMM::map_page(page_addr, (uint32_t)new_frame, curr_vma->flags);
                    return true;
                }
                curr_vma = curr_vma->next;
            }
        }
    }

    return false;
}

uint32_t* VMM::get_current_directory() {
    return (uint32_t*)PAGE_DIR_VADDR;
}

} // namespace re36
