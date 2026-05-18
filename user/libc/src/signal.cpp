#include <signal.h>
#include <sys/syscall.h>

extern "C" __attribute__((naked)) void __vlsmc_signal_trampoline(void) {
    asm volatile(
        "movl $61, %eax\n\t"
        "int $0x80\n\t"
        "1:\n\t"
        "hlt\n\t"
        "jmp 1b\n\t"
    );
}

sighandler_t signal(int sig, sighandler_t func) {
    long old = syscall(SYS_SIGNAL, sig, (long)func, (long)__vlsmc_signal_trampoline);
    if (old == -1) {
        return SIG_ERR;
    }
    return (sighandler_t)old;
}

int raise(int sig) {
    long pid = syscall(SYS_GETPID);
    if (pid == -1) {
        return -1;
    }
    return (int)syscall(SYS_KILL, pid, sig);
}

int kill(int pid, int sig) {
    return (int)syscall(SYS_KILL, pid, sig);
}
