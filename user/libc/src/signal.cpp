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
    struct sigaction act;
    struct sigaction oldact;
    act.sa_handler = func;
    act.sa_mask = 0;
    act.sa_flags = 0;

    if (sigaction(sig, &act, &oldact) != 0) {
        return SIG_ERR;
    }
    return oldact.sa_handler;
}

int sigaction(int sig, const struct sigaction* act, struct sigaction* oldact) {
    return (int)syscall(SYS_SIGACTION, sig, (long)act, (long)oldact,
                        (long)__vlsmc_signal_trampoline);
}

int sigemptyset(sigset_t* set) {
    if (!set) return -1;
    *set = 0;
    return 0;
}

int sigfillset(sigset_t* set) {
    if (!set) return -1;
    *set = 0xFFFFFFFFu;
    *set &= ~(1u << SIGKILL);
    return 0;
}

int sigaddset(sigset_t* set, int sig) {
    if (!set || sig <= 0 || sig >= 32 || sig == SIGKILL) return -1;
    *set |= (1u << sig);
    return 0;
}

int sigdelset(sigset_t* set, int sig) {
    if (!set || sig <= 0 || sig >= 32) return -1;
    *set &= ~(1u << sig);
    return 0;
}

int sigismember(const sigset_t* set, int sig) {
    if (!set || sig <= 0 || sig >= 32) return -1;
    return ((*set & (1u << sig)) != 0) ? 1 : 0;
}

int sigprocmask(int how, const sigset_t* set, sigset_t* oldset) {
    return (int)syscall(SYS_SIGPROCMASK, how, (long)set, (long)oldset);
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
