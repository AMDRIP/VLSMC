#pragma once

#include <stdint.h>

namespace re36 {

struct TSSEntry {
    uint32_t prev_tss;
    uint32_t esp0;      // Стек ядра (Ring 0) — сюда CPU прыгает при прерывании из Ring 3
    uint32_t ss0;       // Сегмент стека ядра (0x10 = Kernel Data)
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax, ecx, edx, ebx;
    uint32_t esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed));

struct GDTEntry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

struct GDTPointer {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

#define GDT_ENTRIES 6

// Селекторы сегментов
#define KERNEL_CS 0x08
#define KERNEL_DS 0x10
#define USER_CS   0x1B  // 0x18 | 3 (RPL=3)
#define USER_DS   0x23  // 0x20 | 3 (RPL=3)
#define TSS_SEG   0x28

class TSS {
public:
    static void init(uint32_t kernel_stack);
    
    static void set_kernel_stack(uint32_t stack_top);
    
    static TSSEntry& get_tss();

private:
    static TSSEntry tss_;
    static GDTEntry gdt_[GDT_ENTRIES];
    static GDTPointer gdt_ptr_;
    
    static void set_gdt_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
    static void write_tss(int index, uint32_t ss0, uint32_t esp0);
    static void flush_gdt();
    static void flush_tss();
};

} // namespace re36
