[BITS 32]
section .text.entry
global _start
extern kernel_main

%define BOOT_INFO_BOOT_STACK_TOP_OFFSET 36
%define DEFAULT_BOOT_STACK_TOP 0x200000

_start:
    cli

    ; DIAGNOSTIC: Print 'D' in protected mode inside kernel
    mov dword [0xB8004], 0x0D440D44

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov eax, [ebx + BOOT_INFO_BOOT_STACK_TOP_OFFSET]
    test eax, eax
    jnz .stack_ready
    mov eax, DEFAULT_BOOT_STACK_TOP
.stack_ready:
    mov esp, eax
    mov ebp, esp

    extern _bss_start
    extern _bss_end
    mov edi, _bss_start
    mov ecx, _bss_end
    sub ecx, edi
    shr ecx, 2
    xor eax, eax
    rep stosd

    push ebx
    call kernel_main

.halt:
    cli
    hlt
    jmp .halt
