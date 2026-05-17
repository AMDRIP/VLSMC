typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef int int32_t;

#define EI_CLASS   4
#define EI_DATA    5
#define EI_VERSION 6

#define ELFCLASS32 1
#define ELFDATA2LSB 1
#define EV_CURRENT 1

#define ET_DYN     3
#define EM_386     3

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
#define DT_RELA     7
#define DT_RELASZ   8
#define DT_RELAENT  9
#define DT_STRSZ    10
#define DT_SYMENT   11
#define DT_INIT     12
#define DT_FINI     13
#define DT_REL      17
#define DT_RELSZ    18
#define DT_RELENT   19
#define DT_PLTREL   20
#define DT_JMPREL   23
#define DT_INIT_ARRAY   25
#define DT_FINI_ARRAY   26
#define DT_INIT_ARRAYSZ 27
#define DT_FINI_ARRAYSZ 28

#define R_386_NONE      0
#define R_386_32        1
#define R_386_PC32      2
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
#define SYS_FSEEK  43
#define SYS_MMAP   12
#define SYS_MUNMAP 13
#define SYS_MPROTECT 58

#define MAP_ANONYMOUS 0x20
#define MAP_PRIVATE   0x02
#define MAP_FIXED     0x10
#define PROT_READ     0x1
#define PROT_WRITE    0x2
#define PROT_EXEC     0x4

#define PAGE_SIZE 4096
#define MAX_OBJECTS 8
#define HEADER_READ_SIZE 4096
#define MAX_PHDRS 32
#define MAX_OBJECT_SPAN 0x04000000
#define LOAD_GUARD_SIZE 0x1000
#define MAX_LOAD_SEGMENTS 8

static inline void sys_print(const char* msg) {
    uint32_t len = 0;
    while (msg[len]) len++;
    asm volatile("int $0x80" :: "a"(1), "b"((uint32_t)msg), "c"(len) : "memory");
}

static inline void sys_exit(int code) {
    asm volatile("int $0x80" :: "a"(0), "b"((uint32_t)code) : "memory");
}

static inline int sys_fopen(const char* path, int mode) {
    int ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_FOPEN), "b"((uint32_t)path), "c"((uint32_t)mode) : "memory");
    return ret;
}

static inline int sys_fread(int fd, void* buf, uint32_t size) {
    int ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_FREAD), "b"((uint32_t)fd), "c"((uint32_t)buf), "d"(size) : "memory");
    return ret;
}

static inline void sys_fclose(int fd) {
    asm volatile("int $0x80" :: "a"(SYS_FCLOSE), "b"((uint32_t)fd) : "memory");
}

static inline uint32_t sys_fsize(int fd) {
    uint32_t ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_FSIZE), "b"((uint32_t)fd) : "memory");
    return ret;
}

static inline int sys_fseek(int fd, int32_t offset, int whence) {
    int ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_FSEEK), "b"((uint32_t)fd), "c"((uint32_t)offset), "d"((uint32_t)whence) : "memory");
    return ret;
}

static inline uint32_t sys_mmap(uint32_t addr, uint32_t len, uint32_t prot, uint32_t flags, int fd) {
    uint32_t ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_MMAP), "b"(addr), "c"(len), "d"(prot), "S"(flags), "D"((uint32_t)fd) : "memory");
    return ret;
}

static inline uint32_t sys_munmap(uint32_t addr, uint32_t len) {
    uint32_t ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_MUNMAP), "b"(addr), "c"(len) : "memory");
    return ret;
}

static inline int sys_mprotect(uint32_t addr, uint32_t len, uint32_t prot) {
    int ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_MPROTECT), "b"(addr), "c"(len), "d"(prot) : "memory");
    return ret;
}

static bool str_eq(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) return false;
        a++; b++;
    }
    return *a == *b;
}

static bool checked_add_u32(uint32_t a, uint32_t b, uint32_t* out) {
    if (a > 0xFFFFFFFFu - b) return false;
    *out = a + b;
    return true;
}

static uint32_t align_down(uint32_t value) {
    return value & ~(PAGE_SIZE - 1);
}

static bool align_up_checked(uint32_t value, uint32_t* out) {
    if (value > 0xFFFFFFFFu - (PAGE_SIZE - 1)) return false;
    *out = (value + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);
    return true;
}

static bool str_in_bounds(const char* base, uint32_t size, uint32_t off) {
    if (!base || off >= size) return false;
    for (uint32_t i = off; i < size; i++) {
        if (base[i] == '\0') return true;
    }
    return false;
}

static bool str_eq_in_bounds(const char* base, uint32_t size, uint32_t off, const char* name) {
    if (!str_in_bounds(base, size, off)) return false;
    uint32_t i = 0;
    while (off + i < size && base[off + i] && name[i]) {
        if (base[off + i] != name[i]) return false;
        i++;
    }
    return (off + i < size && base[off + i] == '\0' && name[i] == '\0');
}

static bool build_library_path(const char* soname, char* out, uint32_t out_size) {
    const char* prefix = "/lib/";
    uint32_t pos = 0;
    while (prefix[pos]) {
        if (pos + 1 >= out_size) return false;
        out[pos] = prefix[pos];
        pos++;
    }

    uint32_t src = 0;
    while (soname[src]) {
        if (pos + 1 >= out_size) return false;
        out[pos++] = soname[src++];
    }
    out[pos] = '\0';
    return src > 0;
}

static bool read_exact_at(int fd, uint32_t offset, void* dst, uint32_t size) {
    if (size == 0) return true;
    if (offset > 0x7FFFFFFFu) return false;
    if (sys_fseek(fd, (int32_t)offset, 0) < 0) return false;

    uint8_t* out = (uint8_t*)dst;
    uint32_t remaining = size;
    while (remaining > 0) {
        int n = sys_fread(fd, out, remaining);
        if (n <= 0) return false;
        out += (uint32_t)n;
        remaining -= (uint32_t)n;
    }
    return true;
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

struct LoadedObject {
    bool        used;
    const char* name;
    uint32_t    load_bias;
    uint32_t    map_start;
    uint32_t    map_end;
    Elf32_Sym*  symtab;
    const char* strtab;
    uint32_t    strtab_size;
    Elf32_Dyn*  dynamic;
    uint32_t    dynamic_count;
    uint32_t    nchain;
    uint32_t    segment_count;
    struct {
        uint32_t start;
        uint32_t end;
        uint32_t prot;
    } segments[MAX_LOAD_SEGMENTS];
};

static LoadedObject g_objects[MAX_OBJECTS];
static int g_num_objects = 0;
static uint32_t g_next_load_addr = 0x60000000;

static bool range_in_object(LoadedObject& obj, uint32_t addr, uint32_t size) {
    uint32_t end = 0;
    if (!checked_add_u32(addr, size, &end)) return false;
    return addr >= obj.map_start && end <= obj.map_end;
}

static bool range_in_span(uint32_t span_start, uint32_t span_end, uint32_t addr, uint32_t size) {
    uint32_t end = 0;
    if (!checked_add_u32(addr, size, &end)) return false;
    return addr >= span_start && end <= span_end;
}

static int find_object(const char* name) {
    for (int i = 0; i < g_num_objects; i++) {
        if (g_objects[i].used && g_objects[i].name && str_eq(g_objects[i].name, name)) {
            return i;
        }
    }
    return -1;
}

static int fail_register_object() {
    if (g_num_objects > 0) {
        g_num_objects--;
        g_objects[g_num_objects].used = false;
    }
    return -1;
}

static int register_object(const char* name, uint32_t bias, uint32_t map_start,
                           uint32_t map_end, Elf32_Dyn* dyn, uint32_t dyn_count) {
    if (g_num_objects >= MAX_OBJECTS) return -1;
    int idx = g_num_objects++;
    LoadedObject& obj = g_objects[idx];
    obj.used = true;
    obj.name = name;
    obj.load_bias = bias;
    obj.map_start = map_start;
    obj.map_end = map_end;
    obj.dynamic = dyn;
    obj.dynamic_count = dyn_count;
    obj.symtab = nullptr;
    obj.strtab = nullptr;
    obj.strtab_size = 0;
    obj.nchain = 0;
    obj.segment_count = 0;
    
    if (!dyn) return idx;
    
    uint32_t* hash_table = nullptr;
    bool saw_null = false;
    
    for (uint32_t i = 0; i < dyn_count; i++) {
        Elf32_Dyn* d = &dyn[i];
        if (d->d_tag == DT_NULL) {
            saw_null = true;
            break;
        }
        switch (d->d_tag) {
            case DT_SYMTAB: obj.symtab = (Elf32_Sym*)(d->d_un.d_ptr + bias); break;
            case DT_STRTAB: obj.strtab = (const char*)(d->d_un.d_ptr + bias); break;
            case DT_STRSZ:  obj.strtab_size = d->d_un.d_val; break;
            case DT_HASH:   hash_table = (uint32_t*)(d->d_un.d_ptr + bias); break;
            case DT_SYMENT:
                if (d->d_un.d_val != sizeof(Elf32_Sym)) return fail_register_object();
                break;
        }
    }

    if (!saw_null) return fail_register_object();
    
    if (hash_table) {
        if (!range_in_object(obj, (uint32_t)hash_table, 8)) return fail_register_object();
        obj.nchain = hash_table[1];
    }

    if ((obj.symtab || obj.strtab) && (!obj.symtab || !obj.strtab || obj.strtab_size == 0 || obj.nchain == 0)) {
        return fail_register_object();
    }

    if (obj.symtab && obj.strtab) {
        uint32_t symtab_bytes = 0;
        if (obj.nchain > 0xFFFFFFFFu / sizeof(Elf32_Sym)) return fail_register_object();
        symtab_bytes = obj.nchain * sizeof(Elf32_Sym);
        if (!range_in_object(obj, (uint32_t)obj.symtab, symtab_bytes)) return fail_register_object();
        if (!range_in_object(obj, (uint32_t)obj.strtab, obj.strtab_size)) return fail_register_object();
    }
    
    return idx;
}

static bool lookup_symbol_in_object(LoadedObject& obj, const char* name, uint32_t* out_addr) {
    if (!obj.symtab || !obj.strtab || obj.nchain == 0) return false;
    
    for (uint32_t i = 1; i < obj.nchain; i++) {
        Elf32_Sym* sym = &obj.symtab[i];
        if (sym->st_shndx == SHN_UNDEF) continue;
        uint8_t bind = ELF32_ST_BIND(sym->st_info);
        if (bind != STB_GLOBAL && bind != STB_WEAK) continue;
        if (!str_eq_in_bounds(obj.strtab, obj.strtab_size, sym->st_name, name)) continue;
        
        if (sym->st_shndx == 0xFFF1) {
            *out_addr = sym->st_value;
        } else {
            *out_addr = sym->st_value + obj.load_bias;
        }
        return true;
    }
    return false;
}

static bool lookup_symbol_global(const char* name, uint32_t* out_addr) {
    for (int i = 0; i < g_num_objects; i++) {
        if (!g_objects[i].used) continue;
        if (lookup_symbol_in_object(g_objects[i], name, out_addr)) return true;
    }
    return false;
}

static bool validate_elf_header(const Elf32_Ehdr* ehdr, uint32_t file_size,
                                uint32_t bytes_read, bool require_dyn) {
    if (ehdr->e_ident[0] != ELFMAG0 || ehdr->e_ident[1] != ELFMAG1 ||
        ehdr->e_ident[2] != ELFMAG2 || ehdr->e_ident[3] != ELFMAG3) {
        return false;
    }
    if (ehdr->e_ident[EI_CLASS] != ELFCLASS32 ||
        ehdr->e_ident[EI_DATA] != ELFDATA2LSB ||
        ehdr->e_ident[EI_VERSION] != EV_CURRENT ||
        ehdr->e_version != EV_CURRENT) {
        return false;
    }
    if (require_dyn && ehdr->e_type != ET_DYN) return false;
    if (ehdr->e_machine != EM_386) return false;
    if (ehdr->e_ehsize != sizeof(Elf32_Ehdr)) return false;
    if (ehdr->e_phentsize != sizeof(Elf32_Phdr)) return false;
    if (ehdr->e_phnum == 0 || ehdr->e_phnum > MAX_PHDRS) return false;

    uint32_t phdr_bytes = 0;
    if (ehdr->e_phnum > 0xFFFFFFFFu / sizeof(Elf32_Phdr)) return false;
    phdr_bytes = ehdr->e_phnum * sizeof(Elf32_Phdr);

    uint32_t phdr_end = 0;
    if (!checked_add_u32(ehdr->e_phoff, phdr_bytes, &phdr_end)) return false;
    if (ehdr->e_phoff < sizeof(Elf32_Ehdr)) return false;
    if (phdr_end > file_size || phdr_end > bytes_read) return false;
    return true;
}

static bool phdr_align_valid(const Elf32_Phdr& ph) {
    if (ph.p_align <= 1) return true;
    if (ph.p_align & (ph.p_align - 1)) return false;
    return ((ph.p_vaddr - ph.p_offset) & (ph.p_align - 1)) == 0;
}

static uint32_t final_prot_from_phdr(const Elf32_Phdr& ph) {
    uint32_t prot = 0;
    if (ph.p_flags & PF_R) prot |= PROT_READ;
    if (ph.p_flags & PF_X) {
        prot |= PROT_READ | PROT_EXEC;
    } else if (ph.p_flags & PF_W) {
        prot |= PROT_READ | PROT_WRITE;
    }
    return prot;
}

static bool parse_dynamic_for_dyn_ptr(Elf32_Phdr* phdrs, uint32_t phnum,
                                      uint32_t phent, uint32_t bias,
                                      Elf32_Dyn** out_dyn,
                                      uint32_t* out_dyn_count) {
    *out_dyn = nullptr;
    *out_dyn_count = 0;
    if (phent != sizeof(Elf32_Phdr) || phnum > MAX_PHDRS) return false;
    for (uint32_t i = 0; i < phnum; i++) {
        Elf32_Phdr* ph = (Elf32_Phdr*)((uint8_t*)phdrs + i * phent);
        if (ph->p_type == PT_DYNAMIC) {
            if (ph->p_filesz == 0 || ph->p_memsz < ph->p_filesz) return false;
            if ((ph->p_memsz % sizeof(Elf32_Dyn)) != 0) return false;
            *out_dyn = (Elf32_Dyn*)(ph->p_vaddr + bias);
            *out_dyn_count = ph->p_memsz / sizeof(Elf32_Dyn);
            return true;
        }
    }
    return true;
}

static bool load_needed_objects(LoadedObject& obj);

static bool load_shared_object(const char* soname) {
    if (find_object(soname) >= 0) return true;

    sys_print("[ld.so] Loading: ");
    sys_print(soname);
    sys_print("\n");

    char path[96];
    if (!build_library_path(soname, path, sizeof(path))) {
        sys_print("[ld.so] ERROR: Bad library name\n");
        return false;
    }

    int fd = sys_fopen(path, 0);
    if (fd < 0) {
        sys_print("[ld.so] ERROR: Cannot open ");
        sys_print(path);
        sys_print("\n");
        return false;
    }

    uint32_t file_size = sys_fsize(fd);
    if (file_size == (uint32_t)-1 || file_size < sizeof(Elf32_Ehdr)) {
        sys_fclose(fd);
        return false;
    }

    uint8_t hdr_buf[HEADER_READ_SIZE];
    uint32_t hdr_to_read = file_size < HEADER_READ_SIZE ? file_size : HEADER_READ_SIZE;
    if (!read_exact_at(fd, 0, hdr_buf, hdr_to_read)) {
        sys_fclose(fd);
        return false;
    }

    int rd = (int)hdr_to_read;
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
    if (!validate_elf_header(ehdr, file_size, hdr_to_read, true)) {
        sys_print("[ld.so] ERROR: Invalid shared object header\n");
        sys_fclose(fd);
        return false;
    }

    Elf32_Phdr* phdrs = (Elf32_Phdr*)(hdr_buf + ehdr->e_phoff);

    uint32_t min_vaddr = 0xFFFFFFFFu;
    uint32_t max_vaddr = 0;
    bool have_load = false;

    for (int p = 0; p < ehdr->e_phnum; p++) {
        Elf32_Phdr& ph = phdrs[p];
        if (ph.p_type != PT_LOAD) continue;
        if (ph.p_memsz == 0) continue;
        if (ph.p_filesz > ph.p_memsz) {
            sys_fclose(fd);
            return false;
        }
        if (!phdr_align_valid(ph)) {
            sys_fclose(fd);
            return false;
        }

        uint32_t file_end = 0;
        uint32_t mem_end = 0;
        if (!checked_add_u32(ph.p_offset, ph.p_filesz, &file_end) ||
            file_end > file_size ||
            !checked_add_u32(ph.p_vaddr, ph.p_memsz, &mem_end)) {
            sys_fclose(fd);
            return false;
        }

        uint32_t seg_start = align_down(ph.p_vaddr);
        uint32_t seg_end = 0;
        if (!align_up_checked(mem_end, &seg_end)) {
            sys_fclose(fd);
            return false;
        }
        if (seg_start < min_vaddr) min_vaddr = seg_start;
        if (seg_end > max_vaddr) max_vaddr = seg_end;
        have_load = true;
    }

    if (!have_load || max_vaddr <= min_vaddr) {
        sys_fclose(fd);
        return false;
    }

    uint32_t total_size = max_vaddr - min_vaddr;
    if (total_size > MAX_OBJECT_SPAN) {
        sys_fclose(fd);
        return false;
    }

    uint32_t load_base = 0;
    if (!align_up_checked(g_next_load_addr, &load_base) || load_base < min_vaddr) {
        sys_fclose(fd);
        return false;
    }
    uint32_t load_bias = load_base - min_vaddr;

    uint32_t map_end = 0;
    if (!checked_add_u32(load_base, total_size, &map_end)) {
        sys_fclose(fd);
        return false;
    }

    uint32_t segment_count = 0;
    LoadedObject segment_template;
    for (uint32_t i = 0; i < MAX_LOAD_SEGMENTS; i++) {
        segment_template.segments[i].start = 0;
        segment_template.segments[i].end = 0;
        segment_template.segments[i].prot = 0;
    }
    for (int p = 0; p < ehdr->e_phnum; p++) {
        Elf32_Phdr& ph = phdrs[p];
        if (ph.p_type != PT_LOAD || ph.p_memsz == 0) continue;
        if (segment_count >= MAX_LOAD_SEGMENTS) {
            sys_fclose(fd);
            return false;
        }

        uint32_t seg_start_raw = 0;
        uint32_t raw_end = 0;
        uint32_t seg_end = 0;
        if (!checked_add_u32(ph.p_vaddr, load_bias, &seg_start_raw) ||
            !checked_add_u32(seg_start_raw, ph.p_memsz, &raw_end) ||
            !align_up_checked(raw_end, &seg_end)) {
            sys_fclose(fd);
            return false;
        }

        segment_template.segments[segment_count].start = align_down(seg_start_raw);
        segment_template.segments[segment_count].end = seg_end;
        segment_template.segments[segment_count].prot = final_prot_from_phdr(ph);
        segment_count++;
    }

    uint32_t base = sys_mmap(load_base, total_size,
                             PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1);
    if (base == (uint32_t)-1 || base != load_base) {
        sys_fclose(fd);
        return false;
    }

    for (int p = 0; p < ehdr->e_phnum; p++) {
        if (phdrs[p].p_type != PT_LOAD) continue;
        if (phdrs[p].p_filesz == 0) continue;

        uint8_t* dest = (uint8_t*)(phdrs[p].p_vaddr + load_bias);
        if (!range_in_span(load_base, map_end, (uint32_t)dest, phdrs[p].p_filesz) ||
            !read_exact_at(fd, phdrs[p].p_offset, dest, phdrs[p].p_filesz)) {
            sys_munmap(load_base, total_size);
            sys_fclose(fd);
            return false;
        }
    }

    sys_fclose(fd);

    uint32_t next_unaligned = 0;
    if (!checked_add_u32(map_end, LOAD_GUARD_SIZE, &next_unaligned) ||
        !align_up_checked(next_unaligned, &g_next_load_addr)) {
        sys_munmap(load_base, total_size);
        return false;
    }

    Elf32_Dyn* so_dyn = nullptr;
    uint32_t so_dyn_count = 0;
    if (!parse_dynamic_for_dyn_ptr(phdrs, ehdr->e_phnum, ehdr->e_phentsize,
                                   load_bias, &so_dyn, &so_dyn_count)) {
        sys_munmap(load_base, total_size);
        return false;
    }
    if (so_dyn && !range_in_span(load_base, map_end, (uint32_t)so_dyn, so_dyn_count * sizeof(Elf32_Dyn))) {
        sys_munmap(load_base, total_size);
        return false;
    }

    int obj_idx = register_object(soname, load_bias, load_base, map_end, so_dyn, so_dyn_count);
    if (obj_idx < 0) {
        sys_munmap(load_base, total_size);
        return false;
    }
    g_objects[obj_idx].segment_count = segment_count;
    for (uint32_t i = 0; i < segment_count; i++) {
        g_objects[obj_idx].segments[i] = segment_template.segments[i];
    }

    sys_print("[ld.so] Loaded ");
    sys_print(soname);
    sys_print(" at ");
    print_hex(load_base);
    sys_print("\n");

    return load_needed_objects(g_objects[obj_idx]);
}

static bool load_needed_objects(LoadedObject& obj) {
    if (!obj.dynamic) return true;
    if (!obj.strtab || obj.strtab_size == 0) return true;

    for (uint32_t i = 0; i < obj.dynamic_count; i++) {
        Elf32_Dyn* d = &obj.dynamic[i];
        if (d->d_tag == DT_NULL) return true;
        if (d->d_tag != DT_NEEDED) continue;

        uint32_t off = d->d_un.d_val;
        if (!str_in_bounds(obj.strtab, obj.strtab_size, off)) {
            sys_print("[ld.so] ERROR: Bad DT_NEEDED string\n");
            return false;
        }

        const char* soname = &obj.strtab[off];
        if (!load_shared_object(soname)) return false;
    }

    sys_print("[ld.so] ERROR: Unterminated dynamic table\n");
    return false;
}

static bool process_relocations(LoadedObject& obj) {
    if (!obj.dynamic) return true;

    Elf32_Rel* rel = nullptr;
    uint32_t   rel_sz = 0;
    Elf32_Rel* jmprel = nullptr;
    uint32_t   jmprel_sz = 0;
    uint32_t   rel_ent = sizeof(Elf32_Rel);
    uint32_t   plt_rel = DT_REL;

    for (uint32_t i = 0; i < obj.dynamic_count; i++) {
        Elf32_Dyn* d = &obj.dynamic[i];
        if (d->d_tag == DT_NULL) break;
        switch (d->d_tag) {
            case DT_REL:      rel = (Elf32_Rel*)(d->d_un.d_ptr + obj.load_bias); break;
            case DT_RELSZ:    rel_sz = d->d_un.d_val; break;
            case DT_JMPREL:   jmprel = (Elf32_Rel*)(d->d_un.d_ptr + obj.load_bias); break;
            case DT_PLTRELSZ: jmprel_sz = d->d_un.d_val; break;
            case DT_RELENT:   rel_ent = d->d_un.d_val; break;
            case DT_PLTREL:   plt_rel = d->d_un.d_val; break;
            case DT_RELA:
            case DT_RELASZ:
            case DT_RELAENT:
                sys_print("[ld.so] ERROR: RELA relocations are not supported\n");
                return false;
        }
    }

    if (rel_ent != sizeof(Elf32_Rel) || plt_rel != DT_REL) {
        sys_print("[ld.so] ERROR: Unsupported relocation entry format\n");
        return false;
    }

    auto do_rels = [&](Elf32_Rel* r, uint32_t sz) -> bool {
        if (!r || !sz) return true;
        if ((sz % sizeof(Elf32_Rel)) != 0 || !range_in_object(obj, (uint32_t)r, sz)) {
            sys_print("[ld.so] ERROR: Bad relocation table\n");
            return false;
        }
        uint32_t cnt = sz / sizeof(Elf32_Rel);
        for (uint32_t i = 0; i < cnt; i++) {
            uint8_t  type    = ELF32_R_TYPE(r[i].r_info);
            uint32_t sym_idx = ELF32_R_SYM(r[i].r_info);
            uint32_t target_addr = r[i].r_offset + obj.load_bias;
            uint32_t* target = (uint32_t*)target_addr;

            if (!range_in_object(obj, target_addr, sizeof(uint32_t))) {
                sys_print("[ld.so] ERROR: Relocation target outside object\n");
                return false;
            }

            switch (type) {
                case R_386_NONE:
                    break;

                case R_386_RELATIVE:
                    *target += obj.load_bias;
                    break;

                case R_386_GLOB_DAT:
                case R_386_JMP_SLOT: {
                    if (!obj.symtab || !obj.strtab || sym_idx >= obj.nchain) return false;
                    Elf32_Sym* sym = &obj.symtab[sym_idx];
                    if (!str_in_bounds(obj.strtab, obj.strtab_size, sym->st_name)) return false;

                    const char* name = &obj.strtab[sym->st_name];
                    uint32_t addr = 0;
                    if (lookup_symbol_global(name, &addr)) {
                        *target = addr;
                    } else if (ELF32_ST_BIND(sym->st_info) == STB_WEAK) {
                        *target = 0;
                    } else {
                        sys_print("[ld.so] UNRESOLVED: ");
                        sys_print(name);
                        sys_print("\n");
                        return false;
                    }
                    break;
                }

                case R_386_32: {
                    if (!obj.symtab || !obj.strtab || sym_idx >= obj.nchain) return false;
                    Elf32_Sym* sym = &obj.symtab[sym_idx];
                    if (!str_in_bounds(obj.strtab, obj.strtab_size, sym->st_name)) return false;

                    const char* name = &obj.strtab[sym->st_name];
                    uint32_t addr = 0;
                    if (lookup_symbol_global(name, &addr)) {
                        *target += addr;
                    } else if (ELF32_ST_BIND(sym->st_info) != STB_WEAK) {
                        sys_print("[ld.so] UNRESOLVED: ");
                        sys_print(name);
                        sys_print("\n");
                        return false;
                    }
                    break;
                }

                case R_386_PC32: {
                    if (!obj.symtab || !obj.strtab || sym_idx >= obj.nchain) return false;
                    Elf32_Sym* sym = &obj.symtab[sym_idx];
                    if (!str_in_bounds(obj.strtab, obj.strtab_size, sym->st_name)) return false;

                    const char* name = &obj.strtab[sym->st_name];
                    uint32_t addr = 0;
                    if (lookup_symbol_global(name, &addr)) {
                        *target += addr - target_addr;
                    } else if (ELF32_ST_BIND(sym->st_info) != STB_WEAK) {
                        sys_print("[ld.so] UNRESOLVED: ");
                        sys_print(name);
                        sys_print("\n");
                        return false;
                    }
                    break;
                }

                default:
                    sys_print("[ld.so] ERROR: Unsupported relocation type\n");
                    return false;
            }
        }
        return true;
    };

    if (!do_rels(rel, rel_sz)) return false;
    if (!do_rels(jmprel, jmprel_sz)) return false;
    return true;
}

typedef void (*EntryFunc)(void);

static bool apply_final_permissions(LoadedObject& obj) {
    for (uint32_t i = 0; i < obj.segment_count; i++) {
        uint32_t len = obj.segments[i].end - obj.segments[i].start;
        if (len == 0) continue;
        if (sys_mprotect(obj.segments[i].start, len, obj.segments[i].prot) < 0) {
            sys_print("[ld.so] ERROR: mprotect failed\n");
            return false;
        }
    }
    return true;
}

static void run_initializers(LoadedObject& obj) {
    if (!obj.dynamic) return;

    EntryFunc init = nullptr;
    EntryFunc* init_array = nullptr;
    uint32_t init_array_sz = 0;

    for (uint32_t i = 0; i < obj.dynamic_count; i++) {
        Elf32_Dyn* d = &obj.dynamic[i];
        if (d->d_tag == DT_NULL) break;
        switch (d->d_tag) {
            case DT_INIT:
                init = (EntryFunc)(d->d_un.d_ptr + obj.load_bias);
                break;
            case DT_INIT_ARRAY:
                init_array = (EntryFunc*)(d->d_un.d_ptr + obj.load_bias);
                break;
            case DT_INIT_ARRAYSZ:
                init_array_sz = d->d_un.d_val;
                break;
        }
    }

    if (init && range_in_object(obj, (uint32_t)init, 1)) {
        init();
    }

    if (init_array && init_array_sz) {
        if ((init_array_sz % sizeof(EntryFunc)) != 0 ||
            !range_in_object(obj, (uint32_t)init_array, init_array_sz)) {
            sys_print("[ld.so] ERROR: Bad init array\n");
            sys_exit(127);
            return;
        }
        uint32_t count = init_array_sz / sizeof(EntryFunc);
        for (uint32_t i = 0; i < count; i++) {
            if (init_array[i]) init_array[i]();
        }
    }
}

extern "C" uint32_t _ld_main(uint32_t* stack_ptr) {
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

    if (at_phdr == 0 || at_entry == 0 || at_phent != sizeof(Elf32_Phdr) ||
        at_phnum == 0 || at_phnum > MAX_PHDRS) {
        sys_print("[ld.so] ERROR: Missing auxv\n");
        sys_exit(127);
        return 0;
    }

    Elf32_Phdr* app_phdrs = (Elf32_Phdr*)at_phdr;

    uint32_t app_map_start = 0xFFFFFFFFu;
    uint32_t app_map_end = 0;
    bool app_have_load = false;
    for (uint32_t i = 0; i < at_phnum; i++) {
        Elf32_Phdr* ph = (Elf32_Phdr*)((uint8_t*)app_phdrs + i * at_phent);
        if (ph->p_type != PT_LOAD || ph->p_memsz == 0) continue;
        if (ph->p_filesz > ph->p_memsz) {
            sys_print("[ld.so] ERROR: Bad app LOAD segment\n");
            sys_exit(127);
            return 0;
        }
        uint32_t mem_end = 0;
        uint32_t seg_end = 0;
        if (!checked_add_u32(ph->p_vaddr, ph->p_memsz, &mem_end) ||
            !align_up_checked(mem_end, &seg_end)) {
            sys_print("[ld.so] ERROR: Bad app address range\n");
            sys_exit(127);
            return 0;
        }
        uint32_t seg_start = align_down(ph->p_vaddr);
        if (seg_start < app_map_start) app_map_start = seg_start;
        if (seg_end > app_map_end) app_map_end = seg_end;
        app_have_load = true;
    }

    if (!app_have_load || app_map_end <= app_map_start) {
        sys_print("[ld.so] ERROR: App has no loadable range\n");
        sys_exit(127);
        return 0;
    }

    Elf32_Dyn* app_dyn = nullptr;
    uint32_t app_dyn_count = 0;
    if (!parse_dynamic_for_dyn_ptr(app_phdrs, at_phnum, at_phent, 0, &app_dyn, &app_dyn_count)) {
        sys_print("[ld.so] ERROR: Bad app dynamic table\n");
        sys_exit(127);
        return 0;
    }

    if (app_dyn && !range_in_span(app_map_start, app_map_end, (uint32_t)app_dyn, app_dyn_count * sizeof(Elf32_Dyn))) {
        sys_print("[ld.so] ERROR: App dynamic table outside LOAD\n");
        sys_exit(127);
        return 0;
    }

    int app_idx = register_object("app", 0, app_map_start, app_map_end, app_dyn, app_dyn_count);
    if (app_idx < 0) {
        sys_print("[ld.so] ERROR: Failed to register app\n");
        sys_exit(127);
        return 0;
    }

    if (!load_needed_objects(g_objects[app_idx])) {
        sys_print("[ld.so] ERROR: Failed to load dependencies\n");
        sys_exit(127);
        return 0;
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
    for (int i = 0; i < g_num_objects; i++) {
        if (g_objects[i].used) {
            if (!process_relocations(g_objects[i])) {
                sys_print("[ld.so] ERROR: Relocation failed\n");
                sys_exit(127);
                return 0;
            }
        }
    }

    for (int i = 1; i < g_num_objects; i++) {
        if (g_objects[i].used && !apply_final_permissions(g_objects[i])) {
            sys_exit(127);
            return 0;
        }
    }

    for (int i = 1; i < g_num_objects; i++) {
        if (g_objects[i].used) {
            run_initializers(g_objects[i]);
        }
    }

    sys_print("[ld.so] Jumping to app entry at ");
    print_hex(at_entry);
    sys_print("\n");

    return at_entry;
}
