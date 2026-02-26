#include "kernel/elf_loader.h"
#include "kernel/elf.h"
#include "kernel/fat16.h"
#include "kernel/vmm.h"
#include "kernel/pmm.h"
#include "kernel/tss.h"
#include "kernel/thread.h"
#include "libc.h"

#include "kernel/kmalloc.h"

namespace re36 {

static bool validate_elf(const Elf32_Ehdr* ehdr) {
    if (ehdr->e_ident[0] != ELFMAG0 || ehdr->e_ident[1] != ELFMAG1 ||
        ehdr->e_ident[2] != ELFMAG2 || ehdr->e_ident[3] != ELFMAG3) {
        printf("[ELF] Invalid magic\n");
        return false;
    }
    if (ehdr->e_type != ET_EXEC) {
        printf("[ELF] Not an executable (type=%d)\n", ehdr->e_type);
        return false;
    }
    if (ehdr->e_machine != EM_386) {
        printf("[ELF] Not i386 (machine=%d)\n", ehdr->e_machine);
        return false;
    }
    return true;
}

static void elf_thread_entry() {
    uint8_t* elf_buffer = (uint8_t*)kmalloc(USER_ELF_MAX_SIZE);
    if (!elf_buffer) {
        printf("[ELF] Failed to alloc buffer\n");
        return;
    }

    int bytes = Fat16::read_file((const char*)threads[current_tid].name, elf_buffer, USER_ELF_MAX_SIZE);
    if (bytes < 0) {
        printf("[ELF] File not found: %s\n", threads[current_tid].name);
        kfree(elf_buffer);
        return;
    }
    printf("[ELF] Loaded %d bytes from FAT\n", bytes);

    uint32_t* new_dir = VMM::create_address_space();
    if (!new_dir) {
        printf("[ELF] Failed to create address space\n");
        kfree(elf_buffer);
        return;
    }

    threads[current_tid].page_directory_phys = new_dir;
    VMM::switch_address_space(new_dir);

    Elf32_Ehdr* ehdr = (Elf32_Ehdr*)elf_buffer;
    if (!validate_elf(ehdr)) return;

    Elf32_Phdr* phdrs = (Elf32_Phdr*)(elf_buffer + ehdr->e_phoff);

    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdrs[i].p_type != PT_LOAD) continue;
        if (phdrs[i].p_memsz == 0) continue;

        uint32_t vaddr_start = phdrs[i].p_vaddr & ~0xFFF;
        uint32_t vaddr_end = (phdrs[i].p_vaddr + phdrs[i].p_memsz + 0xFFF) & ~0xFFF;
        uint32_t pages_needed = (vaddr_end - vaddr_start) / 4096;

        for (uint32_t p = 0; p < pages_needed; p++) {
            uint32_t vaddr = vaddr_start + p * 4096;
            void* frame = PhysicalMemoryManager::alloc_frame();
            if (!frame) {
                printf("[ELF] Out of memory\n");
                return;
            }

            uint32_t flags = PAGE_PRESENT | PAGE_USER;
            if (phdrs[i].p_flags & PF_W) flags |= PAGE_WRITABLE;
            VMM::map_page(vaddr, (uint32_t)frame, flags);

            uint8_t* page_ptr = (uint8_t*)vaddr;
            for (int b = 0; b < 4096; b++) page_ptr[b] = 0;
        }

        uint8_t* seg_src = elf_buffer + phdrs[i].p_offset;
        uint8_t* seg_dst = (uint8_t*)phdrs[i].p_vaddr;
        for (uint32_t b = 0; b < phdrs[i].p_filesz; b++) {
            seg_dst[b] = seg_src[b];
        }
    }

    for (uint32_t p = 0; p < USER_STACK_PAGES; p++) {
        void* frame = PhysicalMemoryManager::alloc_frame();
        if (!frame) {
            printf("[ELF] Out of memory for stack\n");
            return;
        }
        uint32_t vaddr = USER_STACK_TOP - (USER_STACK_PAGES - p) * 4096;
        VMM::map_page(vaddr, (uint32_t)frame, PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);
        uint8_t* page_ptr = (uint8_t*)vaddr;
        for (int b = 0; b < 4096; b++) page_ptr[b] = 0;
    }

    uint32_t entry = ehdr->e_entry;
    uint32_t user_esp = USER_STACK_TOP;

    TSS::set_kernel_stack((uint32_t)(threads[current_tid].stack_base + THREAD_STACK_SIZE));

    kfree(elf_buffer);

    uint8_t* code = (uint8_t*)entry;
    printf("[ELF] Code at entry: %x %x %x %x\n", code[0], code[1], code[2], code[3]);

    // Подготавливаем значения для iret в конкретных регистрах,
    // чтобы компилятор не мог использовать их под свои нужды во время push.
    uint32_t eflags;
    asm volatile("pushf; pop %0" : "=r"(eflags));
    eflags |= 0x200; // Включаем прерывания (IF)

    asm volatile(
        "cli\n\t"
        "mov $0x23, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"

        // Структура стека для iret: SS, ESP, EFLAGS, CS, EIP
        "push $0x23\n\t"      // SS
        "push %%ecx\n\t"      // ESP (передан через ecx)
        "push %%ebx\n\t"      // EFLAGS (передан через ebx)
        "push $0x1B\n\t"      // CS
        "push %%edx\n\t"      // EIP (передан через edx)

        "iret\n\t"
        :: "c"(user_esp), "d"(entry), "b"(eflags)
        : "eax", "memory"
    );
}

bool elf_exec(const char* filename) {
    int tid = thread_create(filename, elf_thread_entry, 5);
    if (tid < 0) {
        printf("[ELF] Failed to create thread\n");
        return false;
    }
    printf("[ELF] Spawned '%s' as TID %d\n", filename, tid);
    return true;
}

} // namespace re36
