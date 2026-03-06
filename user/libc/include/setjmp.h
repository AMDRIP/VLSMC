#pragma once

// x86 jmp_buf requires saving 6 registers: ebx, esp, ebp, esi, edi, eip
typedef int jmp_buf[6]; 

#ifdef __cplusplus
extern "C" {
#endif

int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val);

#ifdef __cplusplus
}
#endif
