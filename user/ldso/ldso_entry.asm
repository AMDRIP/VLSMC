[BITS 32]
section .text
global _start
extern _ld_main

_start:
    ; Pass current stack pointer (with argc/argv/auxv) to _ld_main
    push esp
    call _ld_main
    add esp, 4   ; Restore stack back to argc

    ; _ld_main returns the app entry point in EAX
    test eax, eax
    jz .hang

    ; Jump directly to the app's entry, stack is pristine
    jmp eax

.hang:
    ; Tell kernel to exit on failure just in case
    mov eax, 0
    mov ebx, 127
    int 0x80
    jmp .hang
