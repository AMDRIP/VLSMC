#include "kernel/vmm.h"
#include "kernel/pmm.h"
#include "kernel/spinlock.h"
#include "kernel/thread.h"
#include "kernel/task_scheduler.h"
#include "libc.h"

namespace re36 {

uint32_t* VMM::current_directory_ = nullptr;
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
    cr0 |= 0x80000000; // Бит PG (Paging Enable)
    asm volatile("mov %0, %%cr0" :: "r"(cr0) : "memory");
}

static inline uint32_t read_cr2() {
    uint32_t val;
    asm volatile("mov %%cr2, %0" : "=r"(val));
    return val;
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

    current_directory_ = page_dir;
    current_directory_phys_ = (uint32_t)page_dir;
    kernel_directory_phys_ = current_directory_phys_;

    load_cr3(current_directory_phys_);
    enable_paging();
}

void VMM::map_page(uint32_t virt, uint32_t phys, uint32_t flags) {
    InterruptGuard guard;
    
    uint32_t pd_index = virt >> 22;
    uint32_t pt_index = (virt >> 12) & 0x3FF;

    if (!(current_directory_[pd_index] & PAGE_PRESENT)) {
        uint32_t* new_table = (uint32_t*)PhysicalMemoryManager::alloc_frame();
        if (!new_table) return;
        for (int i = 0; i < PT_ENTRIES; i++) {
            new_table[i] = 0;
        }
        uint32_t pd_flags = PAGE_PRESENT | PAGE_WRITABLE;
        if (flags & PAGE_USER) pd_flags |= PAGE_USER;
        current_directory_[pd_index] = (uint32_t)new_table | pd_flags;
    }

    uint32_t* pt = (uint32_t*)(current_directory_[pd_index] & 0xFFFFF000);
    pt[pt_index] = (phys & 0xFFFFF000) | (flags & 0xFFF) | PAGE_PRESENT;

    invlpg(virt);
}

void VMM::unmap_page(uint32_t virt) {
    InterruptGuard guard;
    
    uint32_t pd_index = virt >> 22;
    uint32_t pt_index = (virt >> 12) & 0x3FF;

    if (!(current_directory_[pd_index] & PAGE_PRESENT)) return;

    uint32_t* pt = (uint32_t*)(current_directory_[pd_index] & 0xFFFFF000);
    pt[pt_index] = 0;

    invlpg(virt);
}

uint32_t VMM::get_physical(uint32_t virt) {
    uint32_t pd_index = virt >> 22;
    uint32_t pt_index = (virt >> 12) & 0x3FF;

    if (!(current_directory_[pd_index] & PAGE_PRESENT)) return 0;

    uint32_t* pt = (uint32_t*)(current_directory_[pd_index] & 0xFFFFF000);
    if (!(pt[pt_index] & PAGE_PRESENT)) return 0;

    return (pt[pt_index] & 0xFFFFF000) | (virt & 0xFFF);
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

    uint32_t kernel_pd_count = KERNEL_SPACE_END >> 22;
    for (uint32_t i = 0; i < kernel_pd_count; i++) {
        new_dir[i] = current_directory_[i];
    }

    return new_dir;
}

void VMM::destroy_address_space(uint32_t* page_dir_phys) {
    if (!page_dir_phys) return;
    if (page_dir_phys == (uint32_t*)kernel_directory_phys_) return; // Нельзя удалять ядро

    InterruptGuard guard;

    uint32_t kernel_pd_count = KERNEL_SPACE_END >> 22;
    for (int i = kernel_pd_count; i < PD_ENTRIES; i++) {
        if (!(page_dir_phys[i] & PAGE_PRESENT)) continue;

        uint32_t* pt = (uint32_t*)(page_dir_phys[i] & 0xFFFFF000);
        for (int j = 0; j < PT_ENTRIES; j++) {
            if (pt[j] & PAGE_PRESENT) {
                // Если фрейм не COW (copy-on-write), мы владеем им и можем освободить
                if (!(pt[j] & PAGE_COW)) {
                    uint32_t phys_frame = pt[j] & 0xFFFFF000;
                    PhysicalMemoryManager::free_frame((void*)phys_frame);
                }
            }
        }
        PhysicalMemoryManager::free_frame((void*)pt); // Удаляем саму PT
    }

    PhysicalMemoryManager::free_frame((void*)page_dir_phys); // Удаляем сам PD
}

void VMM::switch_address_space(uint32_t* page_dir_phys) {
    current_directory_ = page_dir_phys;
    current_directory_phys_ = (uint32_t)page_dir_phys;
    load_cr3(current_directory_phys_);
}

uint32_t* VMM::clone_directory() {
    InterruptGuard guard;
    
    uint32_t* new_dir = (uint32_t*)PhysicalMemoryManager::alloc_frame();
    if (!new_dir) return nullptr;

    for (int i = 0; i < PD_ENTRIES; i++) {
        new_dir[i] = 0;
    }

    uint32_t kernel_pd_count = KERNEL_SPACE_END >> 22;
    for (uint32_t i = 0; i < kernel_pd_count; i++) {
        new_dir[i] = current_directory_[i];
    }

    for (int i = kernel_pd_count; i < PD_ENTRIES; i++) {
        if (!(current_directory_[i] & PAGE_PRESENT)) continue;

        uint32_t* src_pt = (uint32_t*)(current_directory_[i] & 0xFFFFF000);

        uint32_t* new_pt = (uint32_t*)PhysicalMemoryManager::alloc_frame();
        if (!new_pt) continue;

        for (int j = 0; j < PT_ENTRIES; j++) {
            if (!(src_pt[j] & PAGE_PRESENT)) {
                new_pt[j] = 0;
                continue;
            }

            new_pt[j] = (src_pt[j] & 0xFFFFF000) | PAGE_PRESENT | PAGE_USER | PAGE_COW;
            new_pt[j] &= ~PAGE_WRITABLE;

            src_pt[j] = (src_pt[j] & 0xFFFFF000) | PAGE_PRESENT | PAGE_USER | PAGE_COW;
            src_pt[j] &= ~PAGE_WRITABLE;

            uint32_t virt_addr = (i << 22) | (j << 12);
            invlpg(virt_addr);
        }

        new_dir[i] = (uint32_t)new_pt | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
    }

    return new_dir;
}

void VMM::handle_page_fault(uint32_t fault_addr, uint32_t error_code) {
    uint32_t pd_index = fault_addr >> 22;
    uint32_t pt_index = (fault_addr >> 12) & 0x3FF;

    bool is_present = (error_code & 0x1) != 0;
    bool is_write   = (error_code & 0x2) != 0;
    bool is_user    = (error_code & 0x4) != 0;

    if (is_present && is_write) {
        if (current_directory_[pd_index] & PAGE_PRESENT) {
            uint32_t* pt = (uint32_t*)(current_directory_[pd_index] & 0xFFFFF000);
            uint32_t pte = pt[pt_index];

            if (pte & PAGE_COW) {
                uint32_t old_phys = pte & 0xFFFFF000;

                void* new_frame = PhysicalMemoryManager::alloc_frame();
                if (!new_frame) {
                    printf("\n!!! PAGE FAULT: Out of memory for CoW copy at 0x%x !!!\n", fault_addr);
                    while(1) asm volatile("cli; hlt");
                }

                uint8_t* src = (uint8_t*)old_phys;
                uint8_t* dst = (uint8_t*)new_frame;
                for (int i = 0; i < (int)PAGE_SIZE; i++) {
                    dst[i] = src[i];
                }

                pt[pt_index] = (uint32_t)new_frame | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
                pt[pt_index] &= ~PAGE_COW;

                invlpg(fault_addr & 0xFFFFF000);
                return;
            }
        }
    } else if (!is_present) {
        // Ленивое выделение памяти (Lazy Allocation) для кучи
        if (current_tid >= 0 && current_tid < MAX_THREADS) {
            Thread& cur = threads[current_tid];
            if (fault_addr >= cur.heap_start && fault_addr < cur.heap_end) {
                void* new_frame = PhysicalMemoryManager::alloc_frame();
                if (!new_frame) {
                    printf("\n!!! PAGE FAULT: Out of memory for heap at 0x%x !!!\n", fault_addr);
                    while(1) asm volatile("cli; hlt");
                }
                
                uint32_t page_addr = fault_addr & 0xFFFFF000;
                VMM::map_page(page_addr, (uint32_t)new_frame, PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);
                return; // Страница выделена, возвращаемся к выполнению юзер-кода
            }
        }
    }

    printf("\n");
    printf("=== PAGE FAULT ===\n");
    printf("Address: 0x%x\n", fault_addr);
    printf("Error:   %s %s %s\n",
        is_present ? "Protection" : "Not-Present",
        is_write ? "Write" : "Read",
        is_user ? "User" : "Kernel");
    printf("PD Index: %d, PT Index: %d\n", pd_index, pt_index);
    printf("SYSTEM HALTED.\n");

    while (1) asm volatile("cli; hlt");
}

uint32_t* VMM::get_current_directory() {
    return current_directory_;
}

} // namespace re36
