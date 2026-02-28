#include <sys/syscall.h>
#include <stdio.h>

extern "C" void __cxa_pure_virtual() {
    // This function is invoked if a pure virtual function is called somehow.
    // It's a fatal error in C++.
    printf("FATAL: Pure virtual function called in user mode!\n");
    syscall(SYS_EXIT, 1);
    while(1);
}
