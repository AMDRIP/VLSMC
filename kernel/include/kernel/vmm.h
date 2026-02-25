#pragma once

#include <stdint.h>
#include <stddef.h>

extern "C" uint32_t _kernel_start;
extern "C" uint32_t _kernel_end;

namespace re36 {

#define PAGE_SIZE 4096

#define PAGE_PRESENT    0x001
#define PAGE_WRITABLE   0x002
#define PAGE_USER       0x004
#define PAGE_WRITETHROUGH 0x008
#define PAGE_CACHEDISABLE 0x010
#define PAGE_ACCESSED   0x020
#define PAGE_DIRTY      0x040
#define PAGE_COW        0x200   // Бит 9 (доступен для ОС): Copy-on-Write

#define PD_ENTRIES 1024
#define PT_ENTRIES 1024

#define KERNEL_SPACE_END 0x00400000   // Первые 4 MB — ядро (Supervisor-only)

class VMM {
public:
    static void init();
    
    static void map_page(uint32_t virt, uint32_t phys, uint32_t flags);
    
    static void unmap_page(uint32_t virt);
    
    static uint32_t get_physical(uint32_t virt);
    
    static void invalidate_page(uint32_t virt);
    
    static void flush_tlb();
    
    static uint32_t* create_address_space();
    static void destroy_address_space(uint32_t* page_dir_phys);
    
    static void switch_address_space(uint32_t* page_dir_phys);
    
    static uint32_t* clone_directory();
    
    static void handle_page_fault(uint32_t fault_addr, uint32_t error_code);
    
    static uint32_t* get_current_directory();

    static uint32_t kernel_directory_phys_;

private:
    static uint32_t* current_directory_;      // Виртуальный адрес текущего PD
    static uint32_t  current_directory_phys_; // Физический адрес текущего PD
};

} // namespace re36
