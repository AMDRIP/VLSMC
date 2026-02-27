#include "kernel/elf_loader.h"
#include "kernel/elf.h"
#include "kernel/fat16.h"
#include "kernel/vfs.h"
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
    uint8_t* header_buf = (uint8_t*)kmalloc(4096);
    if (!header_buf) {
        printf("[ELF] Failed to alloc header buffer\n");
        return;
    }

    vnode* vn = nullptr;
    if (vfs_resolve_path((const char*)threads[current_tid].name, &vn) != 0 || !vn) {
        printf("[ELF] File not found: %s\n", threads[current_tid].name);
        kfree(header_buf);
        return;
    }

    int bytes = -1;
    if (vn->ops && vn->ops->read) {
        bytes = vn->ops->read(vn, 0, header_buf, 4096);
    }

    if (bytes < (int)sizeof(Elf32_Ehdr)) {
        printf("[ELF] File too small: %s (bytes=%d)\n", threads[current_tid].name, bytes);
        kfree(header_buf);
        return;
    }

    Elf32_Ehdr* ehdr = (Elf32_Ehdr*)header_buf;
    if (!validate_elf(ehdr)) {
        kfree(header_buf);
        return;
    }

    uint32_t* new_dir = VMM::create_address_space();
    if (!new_dir) {
        printf("[ELF] Failed to create address space\n");
        kfree(header_buf);
        return;
    }

    threads[current_tid].page_directory_phys = new_dir;
    VMM::switch_address_space(new_dir);
    
    // Очищаем старые VMA (на всякий случай, хотя они д.б. очищены в thread_init)
    VMA* curr = threads[current_tid].vma_list;
    while (curr) {
        VMA* next = curr->next;
        kfree(curr);
        curr = next;
    }
    threads[current_tid].vma_list = nullptr;

    Elf32_Phdr* phdrs = (Elf32_Phdr*)(header_buf + ehdr->e_phoff);
    uint32_t max_vaddr = 0;

    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdrs[i].p_type != PT_LOAD) continue;
        if (phdrs[i].p_memsz == 0) continue;

        uint32_t vaddr_start = phdrs[i].p_vaddr & ~0xFFF;
        uint32_t vaddr_end = (phdrs[i].p_vaddr + phdrs[i].p_memsz + 0xFFF) & ~0xFFF;
        
        if (vaddr_end > max_vaddr) {
            max_vaddr = vaddr_end;
        }

        uint32_t flags = PAGE_PRESENT | PAGE_USER;
        if (phdrs[i].p_flags & PF_W) flags |= PAGE_WRITABLE;

        VMA* new_vma = (VMA*)kmalloc(sizeof(VMA));
        if (!new_vma) {
            printf("[ELF] Out of memory for VMA structure\n");
            break;
        }

        new_vma->start = vaddr_start;
        new_vma->end = vaddr_end;
        
        uint32_t align_diff = phdrs[i].p_vaddr - vaddr_start;
        new_vma->file_offset = phdrs[i].p_offset - align_diff;
        new_vma->file_size = phdrs[i].p_filesz + align_diff;
        new_vma->flags = flags;
        
        // Вставляем в начало списка VMA текущего потока
        new_vma->next = threads[current_tid].vma_list;
        threads[current_tid].vma_list = new_vma;
    }

    uint32_t entry = ehdr->e_entry;

    kfree(header_buf);

    // Guard gap: оставляем 1 страницу (4096 байт) пустой между кодом/данными и началом кучи
    // Это гарантирует, что heap_start никогда не соприкоснется с VMA
    uint32_t heap_base = (max_vaddr + 0xFFF) & ~0xFFF;
    heap_base += 4096;

    threads[current_tid].heap_start = heap_base;
    threads[current_tid].heap_end = heap_base;
    threads[current_tid].heap_lock = false;

    // Стек выделяем жестко, но оставляем одну страницу невыделенной ПОСЛЕ НЕГО (по адресам ниже)
    // чтобы служить страницей-стражем от Stack Overflow
    for (uint32_t p = 0; p < USER_STACK_PAGES; p++) {
        void* frame = PhysicalMemoryManager::alloc_frame();
        if (!frame) {
            printf("[ELF] Out of memory for stack\n");
            return;
        }
        // vaddr вычисляется в обратную сторону: USER_STACK_TOP, -4096, -8192 ...
        uint32_t vaddr = USER_STACK_TOP - (USER_STACK_PAGES - p) * 4096;
        VMM::map_page(vaddr, (uint32_t)frame, PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);
        uint8_t* page_ptr = (uint8_t*)vaddr;
        for (int b = 0; b < 4096; b++) page_ptr[b] = 0;
    }
    // Страница по адресу: (USER_STACK_TOP - (USER_STACK_PAGES + 1) * 4096) остается НЕ выделенной (Not Present)
    // Если стек достигнет этого адреса, мы получим мгновенный аппаратный сбой защиты!

    uint32_t user_esp = USER_STACK_TOP;
    TSS::set_kernel_stack((uint32_t)(threads[current_tid].stack_base + THREAD_STACK_SIZE));

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
