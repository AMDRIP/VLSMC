typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef int int32_t;

#define AT_NULL    0
#define AT_PHDR    3
#define AT_PHENT   4
#define AT_PHNUM   5
#define AT_PAGESZ  6
#define AT_BASE    7
#define AT_ENTRY   9

#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2

#define DT_NULL     0
#define DT_NEEDED   1
#define DT_PLTRELSZ 2
#define DT_PLTGOT   3
#define DT_HASH     4
#define DT_STRTAB   5
#define DT_SYMTAB   6
#define DT_REL      17
#define DT_RELSZ    18
#define DT_RELENT   19
#define DT_PLTREL   20
#define DT_JMPREL   23

#define R_386_NONE      0
#define R_386_32        1
#define R_386_PC32      2
#define R_386_COPY      5
#define R_386_GLOB_DAT  6
#define R_386_JMP_SLOT  7
#define R_386_RELATIVE  8

#define ELF32_R_SYM(i)    ((i) >> 8)
#define ELF32_R_TYPE(i)   ((uint8_t)(i))

#define STB_GLOBAL 1
#define STB_WEAK   2
#define ELF32_ST_BIND(i) ((i) >> 4)

#define SHN_UNDEF 0

struct Elf32_Phdr {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
};

struct Elf32_Dyn {
    int32_t  d_tag;
    union {
        uint32_t d_val;
        uint32_t d_ptr;
    } d_un;
};

struct Elf32_Sym {
    uint32_t st_name;
    uint32_t st_value;
    uint32_t st_size;
    uint8_t  st_info;
    uint8_t  st_other;
    uint16_t st_shndx;
};

struct Elf32_Rel {
    uint32_t r_offset;
    uint32_t r_info;
};

static inline void sys_print(const char* msg) {
    uint32_t len = 0;
    while (msg[len]) len++;
    asm volatile("int $0x80" :: "a"(1), "b"((uint32_t)msg), "c"(len));
}

static inline void sys_exit(int code) {
    asm volatile("int $0x80" :: "a"(0), "b"((uint32_t)code));
}

static bool str_eq(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) return false;
        a++; b++;
    }
    return *a == *b;
}

static void print_hex(uint32_t val) {
    char buf[11];
    buf[0] = '0'; buf[1] = 'x';
    const char* hex = "0123456789ABCDEF";
    for (int i = 7; i >= 0; i--) {
        buf[2 + (7 - i)] = hex[(val >> (i * 4)) & 0xF];
    }
    buf[10] = '\0';
    sys_print(buf);
}

typedef void (*EntryFunc)(void);

extern "C" void _ld_main(uint32_t* stack_ptr) {
    uint32_t argc = stack_ptr[0];
    uint32_t* argv = &stack_ptr[1];
    uint32_t* envp = &argv[argc + 1];

    uint32_t* p = envp;
    while (*p) p++;
    p++;

    uint32_t at_phdr = 0, at_phnum = 0, at_phent = 0;
    uint32_t at_entry = 0, at_base = 0, at_pagesz = 0;

    while (true) {
        uint32_t type = p[0];
        uint32_t val  = p[1];
        if (type == AT_NULL) break;
        switch (type) {
            case AT_PHDR:    at_phdr = val; break;
            case AT_PHNUM:   at_phnum = val; break;
            case AT_PHENT:   at_phent = val; break;
            case AT_ENTRY:   at_entry = val; break;
            case AT_BASE:    at_base = val; break;
            case AT_PAGESZ:  at_pagesz = val; break;
        }
        p += 2;
    }

    sys_print("[ld.so] Dynamic linker started\n");
    sys_print("[ld.so] AT_ENTRY=");
    print_hex(at_entry);
    sys_print(" AT_PHDR=");
    print_hex(at_phdr);
    sys_print(" AT_BASE=");
    print_hex(at_base);
    sys_print("\n");

    if (at_phdr == 0 || at_entry == 0) {
        sys_print("[ld.so] ERROR: Missing auxv entries\n");
        sys_exit(127);
        return;
    }

    Elf32_Phdr* phdrs = (Elf32_Phdr*)at_phdr;
    Elf32_Dyn* dyn_table = nullptr;

    for (uint32_t i = 0; i < at_phnum; i++) {
        Elf32_Phdr* ph = (Elf32_Phdr*)((uint8_t*)phdrs + i * at_phent);
        if (ph->p_type == PT_DYNAMIC) {
            dyn_table = (Elf32_Dyn*)ph->p_vaddr;
            break;
        }
    }

    if (!dyn_table) {
        sys_print("[ld.so] No PT_DYNAMIC in app, jumping to entry\n");
        EntryFunc entry = (EntryFunc)at_entry;
        entry();
        sys_exit(0);
        return;
    }

    uint32_t* dt_pltgot = nullptr;
    Elf32_Sym* symtab = nullptr;
    const char* strtab = nullptr;
    Elf32_Rel* rel_table = nullptr;
    uint32_t rel_size = 0;
    Elf32_Rel* jmprel_table = nullptr;
    uint32_t jmprel_size = 0;

    for (Elf32_Dyn* d = dyn_table; d->d_tag != DT_NULL; d++) {
        switch (d->d_tag) {
            case DT_PLTGOT:   dt_pltgot = (uint32_t*)d->d_un.d_ptr; break;
            case DT_SYMTAB:   symtab = (Elf32_Sym*)d->d_un.d_ptr; break;
            case DT_STRTAB:   strtab = (const char*)d->d_un.d_ptr; break;
            case DT_REL:      rel_table = (Elf32_Rel*)d->d_un.d_ptr; break;
            case DT_RELSZ:    rel_size = d->d_un.d_val; break;
            case DT_JMPREL:   jmprel_table = (Elf32_Rel*)d->d_un.d_ptr; break;
            case DT_PLTRELSZ: jmprel_size = d->d_un.d_val; break;
        }
    }

    auto process_rels = [&](Elf32_Rel* rels, uint32_t size) {
        if (!rels || size == 0) return;
        int count = size / sizeof(Elf32_Rel);
        for (int i = 0; i < count; i++) {
            uint8_t  type = ELF32_R_TYPE(rels[i].r_info);
            uint32_t sym_idx = ELF32_R_SYM(rels[i].r_info);
            uint32_t* target = (uint32_t*)rels[i].r_offset;

            switch (type) {
                case R_386_RELATIVE:
                    break;

                case R_386_GLOB_DAT:
                case R_386_JMP_SLOT: {
                    if (!symtab || !strtab) break;
                    Elf32_Sym* sym = &symtab[sym_idx];
                    const char* name = &strtab[sym->st_name];

                    if (sym->st_shndx != SHN_UNDEF) {
                        *target = sym->st_value;
                    } else {
                        sys_print("[ld.so] WARN: unresolved symbol: ");
                        sys_print(name);
                        sys_print("\n");
                        *target = 0;
                    }
                    break;
                }

                case R_386_32: {
                    if (!symtab) break;
                    Elf32_Sym* sym = &symtab[sym_idx];
                    if (sym->st_shndx != SHN_UNDEF) {
                        *target += sym->st_value;
                    }
                    break;
                }
            }
        }
    };

    sys_print("[ld.so] Processing relocations...\n");
    process_rels(rel_table, rel_size);
    process_rels(jmprel_table, jmprel_size);

    sys_print("[ld.so] Jumping to app entry at ");
    print_hex(at_entry);
    sys_print("\n");

    EntryFunc entry = (EntryFunc)at_entry;
    entry();
    sys_exit(0);
}
