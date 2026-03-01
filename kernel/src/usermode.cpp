#include "kernel/usermode.h"
#include "kernel/vmm.h"
#include "kernel/pmm.h"
#include "kernel/tss.h"
#include "kernel/thread.h"
#include "libc.h"

namespace re36 {

static uint8_t user_program[] = {
    // --- Настройка сегментов данных (USER_DS = 0x23) ---
    0x66, 0xB8, 0x23, 0x00,         // mov ax, 0x23
    0x8E, 0xD8,                     // mov ds, ax
    0x8E, 0xC0,                     // mov es, ax
    0x8E, 0xE0,                     // mov fs, ax
    0x8E, 0xE8,                     // mov gs, ax

    // --- Строим строку "Hello from Ring 3!\n" на стеке ---
    0x68, 0x33, 0x21, 0x0A, 0x00,   // push 0x000A2133  "3!\n\0"
    0x68, 0x69, 0x6E, 0x67, 0x20,   // push 0x20676E69  "ing "
    0x68, 0x6F, 0x6D, 0x20, 0x52,   // push 0x52206D6F  "om R"
    0x68, 0x6F, 0x20, 0x66, 0x72,   // push 0x7266206F  "o fr"
    0x68, 0x48, 0x65, 0x6C, 0x6C,   // push 0x6C6C6548  "Hell"

    // --- SYS_PRINT(eax=1, ebx=esp, ecx=19) ---
    0xB8, 0x01, 0x00, 0x00, 0x00,   // mov eax, 1
    0x89, 0xE3,                     // mov ebx, esp
    0xB9, 0x13, 0x00, 0x00, 0x00,   // mov ecx, 19
    0xCD, 0x80,                     // int 0x80

    // --- Очистка стека ---
    0x83, 0xC4, 0x14,               // add esp, 20

    // --- Бесконечный цикл: SYS_YIELD ---
    // loop (offset 54):
    0xB8, 0x04, 0x00, 0x00, 0x00,   // mov eax, 4  (SYS_YIELD)
    0xCD, 0x80,                     // int 0x80
    0xEB, 0xF7,                     // jmp loop (-9 → offset 54)
};

void enter_usermode() {
    uint32_t* new_dir = VMM::create_address_space();
    if (!new_dir) {
        printf("[USERMODE] Failed to create address space!\n");
        return;
    }
    threads[current_tid].page_directory_phys = new_dir;
    VMM::switch_address_space(new_dir);

    void* code_frame = PhysicalMemoryManager::alloc_frame();
    void* stack_frame = PhysicalMemoryManager::alloc_frame();

    if (!code_frame || !stack_frame) {
        printf("[USERMODE] Failed to allocate frames!\n");
        return;
    }

    VMM::map_page(USER_CODE_VADDR, (uint32_t)code_frame,
                  PAGE_PRESENT | PAGE_USER);

    VMM::map_page(USER_STACK_VADDR, (uint32_t)stack_frame,
                  PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);

    uint8_t* code_dst = (uint8_t*)USER_CODE_VADDR;
    for (uint32_t i = 0; i < sizeof(user_program); i++) {
        code_dst[i] = user_program[i];
    }

    uint32_t user_esp = USER_STACK_VADDR + USER_STACK_SIZE;

    printf("[USERMODE] Code at 0x%x, Stack at 0x%x\n", USER_CODE_VADDR, user_esp);
    printf("[USERMODE] Jumping to Ring 3...\n");

    TSS::set_kernel_stack((uint32_t)(threads[current_tid].stack_base + THREAD_STACK_SIZE));

    asm volatile(
        "cli\n\t"
        "mov $0x23, %%ax\n\t"   // USER_DS | RPL=3
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"

        "push $0x23\n\t"        // SS  = USER_DS
        "push %0\n\t"           // ESP = user stack top
        "pushfl\n\t"            // EFLAGS
        "pop %%eax\n\t"
        "or $0x200, %%eax\n\t"  // Включить IF (прерывания)
        "push %%eax\n\t"        // EFLAGS с IF=1
        "push $0x1B\n\t"        // CS  = USER_CS | RPL=3
        "push %1\n\t"           // EIP = user code entry

        "iret\n\t"
        ::
        "r"(user_esp),
        "r"((uint32_t)USER_CODE_VADDR)
        : "eax", "memory"
    );

    while (1) asm volatile("hlt");
}

} // namespace re36
