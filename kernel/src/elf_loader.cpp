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
    uint8_t* header_buf = (uint8_t*)kmalloc(4096);
    if (!header_buf) {
        printf("[ELF] Failed to alloc header buffer\n");
        return;
    }

    // Читаем только первую страницу (заголовки ELF)
    int bytes = Fat16::read_file_offset((const char*)threads[current_tid].name, 0, header_buf, 4096);
    if (bytes < sizeof(Elf32_Ehdr)) {
        printf("[ELF] File too small or not found: %s (bytes=%d)\n", threads[current_tid].name, bytes);
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
    
    // Очищаем массив VMA
    for (int i = 0; i < 8; i++) {
        threads[current_tid].vmas[i].active = false;
    }

    Elf32_Phdr* phdrs = (Elf32_Phdr*)(header_buf + ehdr->e_phoff);
    uint32_t max_vaddr = 0;
    int vma_idx = 0;

    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdrs[i].p_type != PT_LOAD) continue;
        if (phdrs[i].p_memsz == 0) continue;
        if (vma_idx >= 8) {
            printf("[ELF] Too many PT_LOAD segments\n");
            break;
        }

        uint32_t vaddr_start = phdrs[i].p_vaddr & ~0xFFF;
        uint32_t vaddr_end = (phdrs[i].p_vaddr + phdrs[i].p_memsz + 0xFFF) & ~0xFFF;
        
        if (vaddr_end > max_vaddr) {
            max_vaddr = vaddr_end;
        }

        uint32_t flags = PAGE_PRESENT | PAGE_USER;
        if (phdrs[i].p_flags & PF_W) flags |= PAGE_WRITABLE;

        threads[current_tid].vmas[vma_idx].start = vaddr_start;
        threads[current_tid].vmas[vma_idx].end = vaddr_end;
        threads[current_tid].vmas[vma_idx].file_offset = phdrs[i].p_offset;
        // Важно: нужно хранить точный виртуальный адрес начала сегмента, чтобы правильно вычислять отступ
        // Сохраним реальный vaddr в start (он выровнен), а file_offset сдвинем
        uint32_t align_diff = phdrs[i].p_vaddr - vaddr_start;
        threads[current_tid].vmas[vma_idx].file_offset = phdrs[i].p_offset - align_diff;
        threads[current_tid].vmas[vma_idx].file_size = phdrs[i].p_filesz + align_diff;
        threads[current_tid].vmas[vma_idx].flags = flags;
        threads[current_tid].vmas[vma_idx].active = true;
        
        vma_idx++;
    }

    uint32_t entry = ehdr->e_entry;

    kfree(header_buf);

    // Guard gap: оставляем 1 страницу (4096 байт) пустой между кодом/данными и началом кучи
    uint32_t heap_base = (max_vaddr + 0xFFF) & ~0xFFF;
    heap_base += 4096;

    threads[current_tid].heap_start = heap_base;
    threads[current_tid].heap_end = heap_base;
    threads[current_tid].heap_lock = false;

    // Стек выделяем жестко (не лениво пока что)
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
