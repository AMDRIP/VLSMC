[BITS 32]
section .text.entry
global _start
extern kernel_main

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

    mov esp, 0x90000
    mov ebp, esp

    extern _bss_start
    extern _bss_end
    mov edi, _bss_start
    mov ecx, _bss_end
    sub ecx, edi
    shr ecx, 2
    xor eax, eax
    rep stosd

    call kernel_main

.halt:
    cli
    hlt
    jmp .halt
