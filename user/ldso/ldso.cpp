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
#define PT_PHDR    6

#define PF_X 1
#define PF_W 2
#define PF_R 4

#define DT_NULL     0
#define DT_NEEDED   1
#define DT_PLTRELSZ 2
#define DT_PLTGOT   3
#define DT_HASH     4
#define DT_STRTAB   5
#define DT_SYMTAB   6
#define DT_STRSZ    10
#define DT_SYMENT   11
#define DT_REL      17
#define DT_RELSZ    18
#define DT_RELENT   19
#define DT_PLTREL   20
#define DT_JMPREL   23

#define R_386_NONE      0
#define R_386_32        1
#define R_386_GLOB_DAT  6
#define R_386_JMP_SLOT  7
#define R_386_RELATIVE  8

#define ELF32_R_SYM(i)    ((i) >> 8)
#define ELF32_R_TYPE(i)   ((uint8_t)(i))
#define ELF32_ST_BIND(i)  ((i) >> 4)

#define SHN_UNDEF 0
#define STB_GLOBAL 1
#define STB_WEAK   2

#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'

struct Elf32_Ehdr {
    uint8_t  e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

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

#define SYS_FOPEN  29
#define SYS_FREAD  30
#define SYS_FCLOSE 32
#define SYS_FSIZE  33
#define SYS_MMAP   12
#define SYS_MUNMAP 13

#define MAP_ANONYMOUS 0x20
#define MAP_PRIVATE   0x02
#define MAP_FIXED     0x10
#define PROT_READ     0x1
#define PROT_WRITE    0x2
#define PROT_EXEC     0x4

static inline void sys_print(const char* msg) {
    uint32_t len = 0;
    while (msg[len]) len++;
    asm volatile("int $0x80" :: "a"(1), "b"((uint32_t)msg), "c"(len));
}

static inline void sys_exit(int code) {
    asm volatile("int $0x80" :: "a"(0), "b"((uint32_t)code));
}

static inline int sys_fopen(const char* path, int mode) {
    int ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_FOPEN), "b"((uint32_t)path), "c"((uint32_t)mode));
    return ret;
}

static inline int sys_fread(int fd, void* buf, uint32_t size) {
    int ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_FREAD), "b"((uint32_t)fd), "c"((uint32_t)buf), "d"(size));
    return ret;
}

static inline void sys_fclose(int fd) {
    asm volatile("int $0x80" :: "a"(SYS_FCLOSE), "b"((uint32_t)fd));
}

static inline uint32_t sys_fsize(int fd) {
    uint32_t ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_FSIZE), "b"((uint32_t)fd));
    return ret;
}

static inline uint32_t sys_mmap(uint32_t addr, uint32_t len, uint32_t prot, uint32_t flags, int fd) {
    uint32_t ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_MMAP), "b"(addr), "c"(len), "d"(prot), "S"(flags), "D"((uint32_t)fd));
    return ret;
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

#define MAX_OBJECTS 8

struct LoadedObject {
    bool        used;
    const char* name;
    uint32_t    load_bias;
    Elf32_Sym*  symtab;
    const char* strtab;
    uint32_t    strtab_size;
    Elf32_Dyn*  dynamic;
    uint32_t    nchain;
};

static LoadedObject g_objects[MAX_OBJECTS];
static int g_num_objects = 0;
static uint32_t g_next_load_addr = 0x60000000;

static int register_object(const char* name, uint32_t bias,
                          Elf32_Dyn* dyn) {
    if (g_num_objects >= MAX_OBJECTS) return -1;
    int idx = g_num_objects++;
    LoadedObject& obj = g_objects[idx];
    obj.used = true;
    obj.name = name;
    obj.load_bias = bias;
    obj.dynamic = dyn;
    obj.symtab = nullptr;
    obj.strtab = nullptr;
    obj.strtab_size = 0;
    obj.nchain = 0;
    
    if (!dyn) return idx;
    
    uint32_t* hash_table = nullptr;
    
    for (Elf32_Dyn* d = dyn; d->d_tag != DT_NULL; d++) {
        switch (d->d_tag) {
            case DT_SYMTAB: obj.symtab = (Elf32_Sym*)(d->d_un.d_ptr + bias); break;
            case DT_STRTAB: obj.strtab = (const char*)(d->d_un.d_ptr + bias); break;
            case DT_STRSZ:  obj.strtab_size = d->d_un.d_val; break;
            case DT_HASH:   hash_table = (uint32_t*)(d->d_un.d_ptr + bias); break;
        }
    }
    
    if (hash_table) {
        obj.nchain = hash_table[1];
    }
    
    return idx;
}

static uint32_t lookup_symbol_in_object(LoadedObject& obj, const char* name) {
    if (!obj.symtab || !obj.strtab || obj.nchain == 0) return 0;
    
    for (uint32_t i = 1; i < obj.nchain; i++) {
        Elf32_Sym* sym = &obj.symtab[i];
        if (sym->st_shndx == SHN_UNDEF) continue;
        uint8_t bind = ELF32_ST_BIND(sym->st_info);
        if (bind != STB_GLOBAL && bind != STB_WEAK) continue;
        
        const char* sym_name = &obj.strtab[sym->st_name];
        if (str_eq(sym_name, name)) {
            return sym->st_value + obj.load_bias;
        }
    }
    return 0;
}

static uint32_t lookup_symbol_global(const char* name) {
    for (int i = 0; i < g_num_objects; i++) {
        if (!g_objects[i].used) continue;
        uint32_t addr = lookup_symbol_in_object(g_objects[i], name);
        if (addr) return addr;
    }
    return 0;
}

static void parse_dynamic_for_dyn_ptr(Elf32_Phdr* phdrs, uint32_t phnum,
                                       uint32_t phent, uint32_t bias,
                                       Elf32_Dyn** out_dyn) {
    *out_dyn = nullptr;
    for (uint32_t i = 0; i < phnum; i++) {
        Elf32_Phdr* ph = (Elf32_Phdr*)((uint8_t*)phdrs + i * phent);
        if (ph->p_type == PT_DYNAMIC) {
            *out_dyn = (Elf32_Dyn*)(ph->p_vaddr + bias);
            return;
        }
    }
}

static bool load_shared_object(const char* soname) {
    sys_print("[ld.so] Loading: ");
    sys_print(soname);
    sys_print("\n");

    char path[64];
    path[0] = '/';
    int i = 0;
    while (soname[i] && i < 62) {
        path[1 + i] = soname[i];
        i++;
    }
    path[1 + i] = '\0';

    int fd = sys_fopen(path, 0);
    if (fd < 0) {
        sys_print("[ld.so] ERROR: Cannot open ");
        sys_print(path);
        sys_print("\n");
        return false;
    }

    uint8_t hdr_buf[512];
    int rd = sys_fread(fd, hdr_buf, 512);
    if (rd < (int)sizeof(Elf32_Ehdr)) {
        sys_fclose(fd);
        return false;
    }

    Elf32_Ehdr* ehdr = (Elf32_Ehdr*)hdr_buf;
    if (ehdr->e_ident[0] != ELFMAG0 || ehdr->e_ident[1] != ELFMAG1 ||
        ehdr->e_ident[2] != ELFMAG2 || ehdr->e_ident[3] != ELFMAG3) {
        sys_fclose(fd);
        return false;
    }

    uint32_t load_bias = g_next_load_addr;

    Elf32_Phdr* phdrs = (Elf32_Phdr*)(hdr_buf + ehdr->e_phoff);

    uint32_t total_size = 0;
    for (int p = 0; p < ehdr->e_phnum; p++) {
        if (phdrs[p].p_type == PT_LOAD) {
            uint32_t seg_end = phdrs[p].p_vaddr + phdrs[p].p_memsz;
            if (seg_end > total_size) total_size = seg_end;
        }
    }
    total_size = (total_size + 0xFFF) & ~0xFFF;

    uint32_t base = sys_mmap(load_bias, total_size,
                             PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1);
    if (base == (uint32_t)-1) {
        sys_fclose(fd);
        return false;
    }

    for (int p = 0; p < ehdr->e_phnum; p++) {
        if (phdrs[p].p_type != PT_LOAD) continue;
        if (phdrs[p].p_filesz == 0) continue;

        sys_fclose(fd);
        fd = sys_fopen(path, 0);
        if (fd < 0) return false;

        uint32_t remaining_skip = phdrs[p].p_offset;
        uint8_t skip_buf[512];
        while (remaining_skip > 0) {
            uint32_t chunk = remaining_skip > 512 ? 512 : remaining_skip;
            int skipped = sys_fread(fd, skip_buf, chunk);
            if (skipped <= 0) break;
            remaining_skip -= skipped;
        }

        uint8_t* dest = (uint8_t*)(phdrs[p].p_vaddr + load_bias);
        uint32_t to_read = phdrs[p].p_filesz;
        while (to_read > 0) {
            int n = sys_fread(fd, dest, to_read);
            if (n <= 0) break;
            dest += n;
            to_read -= n;
        }
    }

    sys_fclose(fd);

    g_next_load_addr = (load_bias + total_size + 0xFFF) & ~0xFFF;
    g_next_load_addr += 0x1000;

    Elf32_Dyn* so_dyn = nullptr;
    for (int p = 0; p < ehdr->e_phnum; p++) {
        if (phdrs[p].p_type == PT_DYNAMIC) {
            so_dyn = (Elf32_Dyn*)(phdrs[p].p_vaddr + load_bias);
            break;
        }
    }

    int obj_idx = register_object(soname, load_bias, so_dyn);
    if (obj_idx < 0) return false;

    sys_print("[ld.so] Loaded ");
    sys_print(soname);
    sys_print(" at ");
    print_hex(load_bias);
    sys_print("\n");

    if (so_dyn) {
        Elf32_Rel* rel = nullptr;
        uint32_t rel_sz = 0;
        for (Elf32_Dyn* d = so_dyn; d->d_tag != DT_NULL; d++) {
            switch (d->d_tag) {
                case DT_REL:  rel = (Elf32_Rel*)(d->d_un.d_ptr + load_bias); break;
                case DT_RELSZ: rel_sz = d->d_un.d_val; break;
            }
        }
        if (rel && rel_sz) {
            int cnt = rel_sz / sizeof(Elf32_Rel);
            for (int r = 0; r < cnt; r++) {
                if (ELF32_R_TYPE(rel[r].r_info) == R_386_RELATIVE) {
                    uint32_t* patch = (uint32_t*)(rel[r].r_offset + load_bias);
                    *patch += load_bias;
                }
            }
        }
    }

    return true;
}

static void process_relocations(LoadedObject& obj) {
    if (!obj.dynamic) return;

    Elf32_Rel* rel = nullptr;
    uint32_t   rel_sz = 0;
    Elf32_Rel* jmprel = nullptr;
    uint32_t   jmprel_sz = 0;

    for (Elf32_Dyn* d = obj.dynamic; d->d_tag != DT_NULL; d++) {
        switch (d->d_tag) {
            case DT_REL:      rel = (Elf32_Rel*)(d->d_un.d_ptr + obj.load_bias); break;
            case DT_RELSZ:    rel_sz = d->d_un.d_val; break;
            case DT_JMPREL:   jmprel = (Elf32_Rel*)(d->d_un.d_ptr + obj.load_bias); break;
            case DT_PLTRELSZ: jmprel_sz = d->d_un.d_val; break;
        }
    }

    auto do_rels = [&](Elf32_Rel* r, uint32_t sz) {
        if (!r || !sz) return;
        int cnt = sz / sizeof(Elf32_Rel);
        for (int i = 0; i < cnt; i++) {
            uint8_t  type    = ELF32_R_TYPE(r[i].r_info);
            uint32_t sym_idx = ELF32_R_SYM(r[i].r_info);
            uint32_t* target = (uint32_t*)(r[i].r_offset + obj.load_bias);

            switch (type) {
                case R_386_RELATIVE:
                    *target += obj.load_bias;
                    break;

                case R_386_GLOB_DAT:
                case R_386_JMP_SLOT: {
                    if (!obj.symtab || !obj.strtab) break;
                    const char* name = &obj.strtab[obj.symtab[sym_idx].st_name];
                    uint32_t addr = lookup_symbol_global(name);
                    if (addr) {
                        *target = addr;
                    } else {
                        sys_print("[ld.so] UNRESOLVED: ");
                        sys_print(name);
                        sys_print("\n");
                        *target = 0;
                    }
                    break;
                }

                case R_386_32: {
                    if (!obj.symtab || !obj.strtab) break;
                    const char* name = &obj.strtab[obj.symtab[sym_idx].st_name];
                    uint32_t addr = lookup_symbol_global(name);
                    if (addr) *target += addr;
                    break;
                }
            }
        }
    };

    do_rels(rel, rel_sz);
    do_rels(jmprel, jmprel_sz);
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

    if (at_phdr == 0 || at_entry == 0) {
        sys_print("[ld.so] ERROR: Missing auxv\n");
        sys_exit(127);
        return;
    }

    Elf32_Phdr* app_phdrs = (Elf32_Phdr*)at_phdr;

    Elf32_Dyn* app_dyn = nullptr;
    parse_dynamic_for_dyn_ptr(app_phdrs, at_phnum, at_phent, 0, &app_dyn);

    register_object("app", 0, app_dyn);

    if (app_dyn) {
        const char* app_strtab = nullptr;
        for (Elf32_Dyn* d = app_dyn; d->d_tag != DT_NULL; d++) {
            if (d->d_tag == DT_STRTAB) {
                app_strtab = (const char*)d->d_un.d_ptr;
                break;
            }
        }

        for (Elf32_Dyn* d = app_dyn; d->d_tag != DT_NULL; d++) {
            if (d->d_tag == DT_NEEDED && app_strtab) {
                const char* soname = &app_strtab[d->d_un.d_val];
                
                bool already = false;
                for (int i = 0; i < g_num_objects; i++) {
                    if (g_objects[i].used && g_objects[i].name && str_eq(g_objects[i].name, soname)) {
                        already = true;
                        break;
                    }
                }
                if (!already) {
                    load_shared_object(soname);
                }
            }
        }
    }

    sys_print("[ld.so] Loaded objects: ");
    for (int i = 0; i < g_num_objects; i++) {
        if (g_objects[i].used && g_objects[i].name) {
            sys_print(g_objects[i].name);
            sys_print(" ");
        }
    }
    sys_print("\n");

    sys_print("[ld.so] Processing relocations...\n");
    process_relocations(g_objects[0]);

    sys_print("[ld.so] Jumping to app entry at ");
    print_hex(at_entry);
    sys_print("\n");

    EntryFunc entry = (EntryFunc)at_entry;
    entry();
    sys_exit(0);
}
