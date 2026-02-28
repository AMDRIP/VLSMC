[BITS 32]
section .text
global _start
extern main
extern __libc_start_main

_start:
    ; ESP here points to argc, followed by argv pointers
    mov ecx, [esp]      ; ecx = argc
    lea edx, [esp + 4]  ; edx = argv

    ; Align the stack to 16 bytes for optimal execution and ABI compliance
    and esp, 0xFFFFFFF0

    ; Push standard arguments for __libc_start_main right-to-left
    push esp            ; stack_end (highest aligned address of stack)
    push 0              ; rtld_fini
    push 0              ; fini
    push 0              ; init
    push edx            ; argv
    push ecx            ; argc
    push main           ; function pointer to application main()

    call __libc_start_main

.hang:
    jmp .hang
