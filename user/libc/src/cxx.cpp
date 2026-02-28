#include <sys/syscall.h>
#include <stdio.h>

extern "C" void* __dso_handle __attribute__((__weak__)) = nullptr;

extern "C" int __cxa_atexit(void (*destructor)(void*), void* arg, void* dso) {
    (void)destructor;
    (void)arg;
    (void)dso;
    return 0;
}

extern "C" void __cxa_pure_virtual() {
    printf("FATAL: Pure virtual function called in user mode!\n");
    syscall(SYS_EXIT, 1);
    while(1);
}
