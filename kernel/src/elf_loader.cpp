#include "kernel/elf_loader.h"
#include "kernel/elf.h"
#include "kernel/vfs.h"
#include "kernel/vmm.h"
#include "kernel/pmm.h"
#include "kernel/tss.h"
#include "kernel/thread.h"
#include "libc.h"

#include "kernel/kmalloc.h"

namespace re36 {

#define PIE_LOAD_BASE    0x40000000
#define INTERP_LOAD_BASE 0x50000000

static bool validate_elf(const Elf32_Ehdr* ehdr) {
    if (ehdr->e_ident[0] != ELFMAG0 || ehdr->e_ident[1] != ELFMAG1 ||
        ehdr->e_ident[2] != ELFMAG2 || ehdr->e_ident[3] != ELFMAG3) {
        printf("[ELF] Invalid magic\n");
        return false;
    }
    if (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN) {
        printf("[ELF] Unsupported ELF type (type=%d)\n", ehdr->e_type);
        return false;
    }
    if (ehdr->e_machine != EM_386) {
        printf("[ELF] Not i386 (machine=%d)\n", ehdr->e_machine);
        return false;
    }
    return true;
}

static void apply_relocations(vnode* vn, Elf32_Phdr* phdrs, int phnum,
                              uint32_t load_bias, uint8_t* header_buf, int header_size) {
    Elf32_Phdr* dyn_phdr = nullptr;
    for (int i = 0; i < phnum; i++) {
        if (phdrs[i].p_type == PT_DYNAMIC) {
            dyn_phdr = &phdrs[i];
            break;
        }
    }
    if (!dyn_phdr) return;

    uint32_t dyn_offset = dyn_phdr->p_offset;
    uint32_t dyn_size = dyn_phdr->p_filesz;

    uint8_t* dyn_buf = nullptr;
    bool dyn_from_header = false;

    if (dyn_offset + dyn_size <= (uint32_t)header_size) {
        dyn_buf = header_buf + dyn_offset;
        dyn_from_header = true;
    } else {
        dyn_buf = (uint8_t*)kmalloc(dyn_size);
        if (!dyn_buf) return;
        if (vn->ops && vn->ops->read) {
            vn->ops->read(vn, dyn_offset, dyn_buf, dyn_size);
        }
    }

    Elf32_Dyn* dyn = (Elf32_Dyn*)dyn_buf;
    int dyn_count = dyn_size / sizeof(Elf32_Dyn);

    uint32_t rel_vaddr = 0, rel_size = 0;
    uint32_t jmprel_vaddr = 0, jmprel_size = 0;

    for (int i = 0; i < dyn_count; i++) {
        if (dyn[i].d_tag == DT_NULL) break;
        switch (dyn[i].d_tag) {
            case DT_REL:      rel_vaddr = dyn[i].d_un.d_ptr; break;
            case DT_RELSZ:    rel_size = dyn[i].d_un.d_val; break;
            case DT_JMPREL:   jmprel_vaddr = dyn[i].d_un.d_ptr; break;
            case DT_PLTRELSZ: jmprel_size = dyn[i].d_un.d_val; break;
        }
    }

    if (!dyn_from_header) kfree(dyn_buf);

    auto vaddr_to_file_offset = [&](uint32_t vaddr) -> uint32_t {
        for (int i = 0; i < phnum; i++) {
            if (phdrs[i].p_type != PT_LOAD) continue;
            if (vaddr >= phdrs[i].p_vaddr &&
                vaddr < phdrs[i].p_vaddr + phdrs[i].p_filesz) {
                return phdrs[i].p_offset + (vaddr - phdrs[i].p_vaddr);
            }
        }
        return 0;
    };

    auto process_rel_table = [&](uint32_t table_vaddr, uint32_t table_size) {
        if (table_vaddr == 0 || table_size == 0) return;

        uint32_t file_off = vaddr_to_file_offset(table_vaddr);
        if (file_off == 0) return;

        uint8_t* rel_buf = nullptr;
        bool rel_from_header = false;

        if (file_off + table_size <= (uint32_t)header_size) {
            rel_buf = header_buf + file_off;
            rel_from_header = true;
        } else {
            rel_buf = (uint8_t*)kmalloc(table_size);
            if (!rel_buf) return;
            if (vn->ops && vn->ops->read) {
                vn->ops->read(vn, file_off, rel_buf, table_size);
            }
        }

        int rel_count = table_size / sizeof(Elf32_Rel);
        Elf32_Rel* rels = (Elf32_Rel*)rel_buf;

        for (int i = 0; i < rel_count; i++) {
            uint8_t type = ELF32_R_TYPE(rels[i].r_info);

            if (type == R_386_RELATIVE) {
                uint32_t target_vaddr = rels[i].r_offset + load_bias;
                uint32_t page_addr = target_vaddr & ~0xFFF;

                uint32_t phys = VMM::get_physical(page_addr);
                if (!phys) {
                    void* frame = PhysicalMemoryManager::alloc_frame();
                    if (!frame) continue;

                    uint8_t* fp = (uint8_t*)frame;
                    for (int b = 0; b < 4096; b++) fp[b] = 0;

                    VMA* v = threads[current_tid].vma_list;
                    while (v) {
                        if (page_addr >= v->start && page_addr < v->end) {
                            uint32_t off_in_vma = page_addr - v->start;
                            uint32_t foff = v->file_offset + off_in_vma;
                            uint32_t fdata_end = v->file_size;
                            if (off_in_vma < fdata_end) {
                                uint32_t rsz = 4096;
                                if (off_in_vma + 4096 > fdata_end) rsz = fdata_end - off_in_vma;
                                if (vn->ops && vn->ops->read)
                                    vn->ops->read(vn, foff, fp, rsz);
                            }
                            break;
                        }
                        v = v->next;
                    }

                    VMM::map_page(page_addr, (uint32_t)frame,
                                  PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);
                }

                uint32_t* patch = (uint32_t*)target_vaddr;
                *patch += load_bias;
            }
        }

        if (!rel_from_header) kfree(rel_buf);
    };

    process_rel_table(rel_vaddr, rel_size);
    process_rel_table(jmprel_vaddr, jmprel_size);
}

struct LoadedElf {
    uint32_t entry;
    uint32_t phdr_vaddr;
    uint32_t phnum;
    uint32_t phent;
    uint32_t load_bias;
    uint32_t max_vaddr;
};

static bool load_elf_segments(vnode* vn, uint8_t* header_buf, int header_size,
                              uint32_t load_bias, LoadedElf* out) {
    Elf32_Ehdr* ehdr = (Elf32_Ehdr*)header_buf;
    Elf32_Phdr* phdrs = (Elf32_Phdr*)(header_buf + ehdr->e_phoff);

    out->entry = ehdr->e_entry + load_bias;
    out->phnum = ehdr->e_phnum;
    out->phent = ehdr->e_phentsize;
    out->load_bias = load_bias;
    out->phdr_vaddr = 0;
    out->max_vaddr = 0;

    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdrs[i].p_type == PT_PHDR) {
            out->phdr_vaddr = phdrs[i].p_vaddr + load_bias;
        }
    }

    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdrs[i].p_type != PT_LOAD) continue;
        if (phdrs[i].p_memsz == 0) continue;

        uint32_t biased_vaddr = phdrs[i].p_vaddr + load_bias;
        uint32_t vaddr_start = biased_vaddr & ~0xFFF;
        uint32_t vaddr_end = (biased_vaddr + phdrs[i].p_memsz + 0xFFF) & ~0xFFF;

        if (vaddr_end > out->max_vaddr) {
            out->max_vaddr = vaddr_end;
        }

        uint32_t flags = PAGE_PRESENT | PAGE_USER;
        if (phdrs[i].p_flags & PF_W) flags |= PAGE_WRITABLE;

        VMA* new_vma = (VMA*)kmalloc(sizeof(VMA));
        if (!new_vma) {
            printf("[ELF] Out of memory for VMA structure\n");
            return false;
        }

        new_vma->start = vaddr_start;
        new_vma->end = vaddr_end;

        uint32_t align_diff = biased_vaddr - vaddr_start;
        new_vma->file_offset = phdrs[i].p_offset - align_diff;
        new_vma->file_size = phdrs[i].p_filesz + align_diff;
        new_vma->flags = flags;
        new_vma->type = VMA_TYPE_FILE;
        new_vma->file_vnode = vn;

        new_vma->next = threads[current_tid].vma_list;
        threads[current_tid].vma_list = new_vma;
    }

    if (out->phdr_vaddr == 0) {
        for (int i = 0; i < ehdr->e_phnum; i++) {
            if (phdrs[i].p_type != PT_LOAD) continue;
            if (ehdr->e_phoff >= phdrs[i].p_offset &&
                ehdr->e_phoff < phdrs[i].p_offset + phdrs[i].p_filesz) {
                out->phdr_vaddr = phdrs[i].p_vaddr + load_bias +
                                  (ehdr->e_phoff - phdrs[i].p_offset);
                break;
            }
        }
    }

    if (ehdr->e_type == ET_DYN && load_bias != 0) {
        apply_relocations(vn, phdrs, ehdr->e_phnum, load_bias, header_buf, header_size);
    }

    return true;
}

static void push_auxv(uint32_t* esp, uint32_t type, uint32_t val) {
    *esp -= 4;
    *(uint32_t*)(*esp) = val;
    *esp -= 4;
    *(uint32_t*)(*esp) = type;
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

    uint32_t load_bias = 0;
    if (ehdr->e_type == ET_DYN) {
        load_bias = PIE_LOAD_BASE;
    }

    uint32_t* new_dir = VMM::create_address_space();
    if (!new_dir) {
        printf("[ELF] Failed to create address space\n");
        kfree(header_buf);
        return;
    }

    threads[current_tid].page_directory_phys = new_dir;
    VMM::switch_address_space(new_dir);

    VMA* curr = threads[current_tid].vma_list;
    while (curr) {
        VMA* next = curr->next;
        kfree(curr);
        curr = next;
    }
    threads[current_tid].vma_list = nullptr;

    Elf32_Phdr* phdrs = (Elf32_Phdr*)(header_buf + ehdr->e_phoff);

    char interp_path[64];
    interp_path[0] = '\0';
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdrs[i].p_type == PT_INTERP && phdrs[i].p_filesz < 64) {
            if (vn->ops && vn->ops->read) {
                vn->ops->read(vn, phdrs[i].p_offset, (uint8_t*)interp_path, phdrs[i].p_filesz);
                interp_path[phdrs[i].p_filesz] = '\0';
                if (phdrs[i].p_filesz > 0 && interp_path[phdrs[i].p_filesz - 1] == '\0') {
                }
            }
            break;
        }
    }

    LoadedElf app_info = {};
    if (!load_elf_segments(vn, header_buf, bytes, load_bias, &app_info)) {
        kfree(header_buf);
        return;
    }

    uint32_t max_vaddr = app_info.max_vaddr;
    uint32_t final_entry = app_info.entry;
    uint32_t interp_base = 0;
    bool has_interp = (interp_path[0] != '\0');

    LoadedElf interp_info = {};

    if (has_interp) {
        printf("[ELF] PT_INTERP: %s\n", interp_path);

        vnode* interp_vn = nullptr;
        if (vfs_resolve_path(interp_path, &interp_vn) != 0 || !interp_vn) {
            printf("[ELF] Interpreter not found: %s\n", interp_path);
            kfree(header_buf);
            return;
        }

        uint8_t* interp_hdr = (uint8_t*)kmalloc(4096);
        if (!interp_hdr) {
            printf("[ELF] Failed to alloc interpreter header\n");
            kfree(header_buf);
            return;
        }

        int interp_bytes = -1;
        if (interp_vn->ops && interp_vn->ops->read) {
            interp_bytes = interp_vn->ops->read(interp_vn, 0, interp_hdr, 4096);
        }

        if (interp_bytes < (int)sizeof(Elf32_Ehdr)) {
            printf("[ELF] Interpreter too small\n");
            kfree(interp_hdr);
            kfree(header_buf);
            return;
        }

        Elf32_Ehdr* interp_ehdr = (Elf32_Ehdr*)interp_hdr;
        if (!validate_elf(interp_ehdr)) {
            kfree(interp_hdr);
            kfree(header_buf);
            return;
        }

        interp_base = INTERP_LOAD_BASE;

        if (!load_elf_segments(interp_vn, interp_hdr, interp_bytes, interp_base, &interp_info)) {
            kfree(interp_hdr);
            kfree(header_buf);
            return;
        }

        if (interp_info.max_vaddr > max_vaddr) {
            max_vaddr = interp_info.max_vaddr;
        }

        final_entry = interp_info.entry;

        kfree(interp_hdr);
        printf("[ELF] Interpreter loaded at 0x%x, entry=0x%x\n", interp_base, final_entry);
    }

    kfree(header_buf);

    uint32_t heap_base = (max_vaddr + 0xFFF) & ~0xFFF;
    heap_base += 4096;

    threads[current_tid].heap_start = heap_base;
    threads[current_tid].heap_end = heap_base;
    threads[current_tid].heap_lock = false;

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

    push_auxv(&user_esp, AT_NULL, 0);
    push_auxv(&user_esp, AT_PAGESZ, 4096);

    if (has_interp) {
        push_auxv(&user_esp, AT_BASE, interp_base);
    }

    push_auxv(&user_esp, AT_ENTRY, app_info.entry);
    push_auxv(&user_esp, AT_PHNUM, app_info.phnum);
    push_auxv(&user_esp, AT_PHENT, app_info.phent);
    push_auxv(&user_esp, AT_PHDR, app_info.phdr_vaddr);

    user_esp -= 4;
    *(uint32_t*)user_esp = 0;

    user_esp -= 4;
    *(uint32_t*)user_esp = 0;

    user_esp -= 4;
    *(uint32_t*)user_esp = 0;

    TSS::set_kernel_stack((uint32_t)(threads[current_tid].stack_base + THREAD_STACK_SIZE));

    uint32_t eflags;
    asm volatile("pushf; pop %0" : "=r"(eflags));
    eflags |= 0x200;

    asm volatile(
        "cli\n\t"
        "mov $0x23, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"

        "push $0x23\n\t"
        "push %%ecx\n\t"
        "push %%ebx\n\t"
        "push $0x1B\n\t"
        "push %%edx\n\t"

        "iret\n\t"
        :: "c"(user_esp), "d"(final_entry), "b"(eflags)
        : "eax", "memory"
    );
}

int elf_exec(const char* filename) {
    int tid = thread_create(filename, elf_thread_entry, 5);
    if (tid < 0) {
        printf("[ELF] Failed to create thread\n");
        return -1;
    }
    printf("[ELF] Spawned '%s' as TID %d\n", filename, tid);
    return tid;
}

} // namespace re36
