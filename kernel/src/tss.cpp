#include "kernel/tss.h"
#include "libc.h"

namespace re36 {

TSSEntry TSS::tss_;
GDTEntry TSS::gdt_[GDT_ENTRIES];
GDTPointer TSS::gdt_ptr_;

void TSS::set_gdt_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt_[index].base_low    = (base & 0xFFFF);
    gdt_[index].base_middle = (base >> 16) & 0xFF;
    gdt_[index].base_high   = (base >> 24) & 0xFF;
    
    gdt_[index].limit_low   = (limit & 0xFFFF);
    gdt_[index].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    
    gdt_[index].access      = access;
}

void TSS::write_tss(int index, uint32_t ss0, uint32_t esp0) {
    uint32_t base = (uint32_t)&tss_;
    uint32_t limit = sizeof(TSSEntry) - 1;

    set_gdt_entry(index, base, limit, 0xE9, 0x00);
    // 0xE9 = Present(1) | DPL=3(11) | Type=Available 32-bit TSS(01001)
    // DPL=3 позволяет переключение из Ring 3

    for (uint32_t i = 0; i < sizeof(TSSEntry); i++) {
        ((uint8_t*)&tss_)[i] = 0;
    }
    
    tss_.ss0  = ss0;
    tss_.esp0 = esp0;
    
    tss_.cs = KERNEL_CS;
    tss_.ss = KERNEL_DS;
    tss_.ds = KERNEL_DS;
    tss_.es = KERNEL_DS;
    tss_.fs = KERNEL_DS;
    tss_.gs = KERNEL_DS;
    
    tss_.iomap_base = sizeof(TSSEntry);
}

void TSS::flush_gdt() {
    asm volatile(
        "lgdt (%0)\n\t"
        "mov $0x10, %%ax\n\t"  // Kernel Data
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        "mov %%ax, %%ss\n\t"
        "ljmp $0x08, $1f\n\t"  // Far jump перезагружает CS
        "1:\n\t"
        :: "r"(&gdt_ptr_)
        : "eax", "memory"
    );
}

void TSS::flush_tss() {
    asm volatile(
        "mov $0x28, %%ax\n\t"   // TSS selector
        "ltr %%ax\n\t"
        ::: "eax"
    );
}

void TSS::init(uint32_t kernel_stack) {
    gdt_ptr_.limit = sizeof(GDTEntry) * GDT_ENTRIES - 1;
    gdt_ptr_.base = (uint32_t)&gdt_;

    // 0: Null descriptor
    set_gdt_entry(0, 0, 0, 0, 0);
    
    // 1: Kernel Code (0x08) — Ring 0, Exec/Read
    set_gdt_entry(1, 0, 0xFFFFF, 0x9A, 0xCF);
    
    // 2: Kernel Data (0x10) — Ring 0, Read/Write
    set_gdt_entry(2, 0, 0xFFFFF, 0x92, 0xCF);
    
    // 3: User Code (0x18) — Ring 3, Exec/Read
    set_gdt_entry(3, 0, 0xFFFFF, 0xFA, 0xCF);
    
    // 4: User Data (0x20) — Ring 3, Read/Write
    set_gdt_entry(4, 0, 0xFFFFF, 0xF2, 0xCF);
    
    // 5: TSS (0x28)
    write_tss(5, KERNEL_DS, kernel_stack);

    flush_gdt();
    flush_tss();
}

void TSS::set_kernel_stack(uint32_t stack_top) {
    tss_.esp0 = stack_top;
}

TSSEntry& TSS::get_tss() {
    return tss_;
}

} // namespace re36
