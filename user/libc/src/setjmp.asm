[BITS 32]

global setjmp
global longjmp

; int setjmp(jmp_buf env);
; env is a pointer to int[6]
; layout: [0]=ebx, [1]=esp, [2]=ebp, [3]=esi, [4]=edi, [5]=eip
setjmp:
    mov eax, [esp + 4]   ; env pointer
    mov [eax + 0], ebx
    
    ; The ESP as it was before the CALL instruction pushed the return address:
    lea ecx, [esp + 4]
    mov [eax + 4], ecx
    
    mov [eax + 8], ebp
    mov [eax + 12], esi
    mov [eax + 16], edi
    
    ; The return address is exactly at [esp] right now
    mov ecx, [esp]
    mov [eax + 20], ecx
    
    xor eax, eax         ; return 0 for first call
    ret

; void longjmp(jmp_buf env, int val);
longjmp:
    mov edx, [esp + 4]   ; env pointer
    mov eax, [esp + 8]   ; val
    
    ; According to POSIX, if val == 0, longjmp returns 1 instead
    test eax, eax
    jnz .val_ok
    mov eax, 1
.val_ok:

    mov ebx, [edx + 0]
    mov esp, [edx + 4]
    mov ebp, [edx + 8]
    mov esi, [edx + 12]
    mov edi, [edx + 16]
    
    ; The EIP we jump to
    mov ecx, [edx + 20]
    
    ; Jump to the saved EIP
    jmp ecx
