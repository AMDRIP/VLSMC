[BITS 32]
section .text
global _start
extern _ld_main

_start:
    mov ebp, esp
    push ebp
    call _ld_main

    mov eax, 0
    mov ebx, 127
    int 0x80
.hang:
    jmp .hang
