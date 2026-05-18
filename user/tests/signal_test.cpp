#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

static volatile sig_atomic_t seen_sigint = 0;
static volatile sig_atomic_t seen_sigusr1 = 0;
static volatile sig_atomic_t seen_sigchld = 0;

static void on_sigint(int sig) {
    seen_sigint = sig;
    printf("[handler] SIGINT handler ran with sig=%d\n", sig);
}

static void on_sigusr1(int sig) {
    seen_sigusr1 = sig;
    printf("[handler] SIGUSR1 unblocked with sig=%d\n", sig);
}

static void on_sigchld(int sig) {
    seen_sigchld = sig;
}

int main() {
    printf("=== SIGNAL TEST ===\n");
    printf("[proc] pid=%d ppid=%d pgrp=%d\n", (int)getpid(), (int)getppid(), (int)getpgrp());

    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_handler = on_sigint;
    act.sa_flags = 0;
    if (sigaction(SIGINT, &act, nullptr) != 0) {
        printf("[FAIL] sigaction(SIGINT) failed\n");
        return 1;
    }

    if (raise(SIGINT) != 0) {
        printf("[FAIL] raise(SIGINT) failed\n");
        return 2;
    }

    if (seen_sigint != SIGINT) {
        printf("[FAIL] handler did not run, seen=%d\n", (int)seen_sigint);
        return 3;
    }
    printf("[OK] handler delivery returned to user code\n");

    sigemptyset(&act.sa_mask);
    act.sa_handler = on_sigusr1;
    act.sa_flags = 0;
    if (sigaction(SIGUSR1, &act, nullptr) != 0) {
        printf("[FAIL] sigaction(SIGUSR1) failed\n");
        return 4;
    }

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    if (sigprocmask(SIG_BLOCK, &mask, nullptr) != 0) {
        printf("[FAIL] SIG_BLOCK failed\n");
        return 5;
    }
    raise(SIGUSR1);
    if (seen_sigusr1 != 0) {
        printf("[FAIL] blocked SIGUSR1 was delivered early\n");
        return 6;
    }
    if (sigprocmask(SIG_UNBLOCK, &mask, nullptr) != 0 || seen_sigusr1 != SIGUSR1) {
        printf("[FAIL] pending SIGUSR1 was not delivered on unblock\n");
        return 7;
    }
    printf("[OK] signal mask defers and releases pending signals\n");

    sigemptyset(&act.sa_mask);
    act.sa_handler = on_sigchld;
    act.sa_flags = 0;
    if (sigaction(SIGCHLD, &act, nullptr) != 0) {
        printf("[FAIL] sigaction(SIGCHLD) failed\n");
        return 8;
    }

    int pid = fork();
    if (pid < 0) {
        printf("[FAIL] fork failed\n");
        return 9;
    }

    if (pid == 0) {
        signal(SIGINT, SIG_DFL);
        printf("[child] raising default SIGINT; this line should be the last child output\n");
        raise(SIGINT);
        printf("[FAIL] child survived default SIGINT\n");
        return 5;
    }

    int status = 0;
    int child = wait(&status);
    printf("[parent] child=%d status=0x%x signaled=%d term_sig=%d chld=%d\n",
           child, status, WIFSIGNALED(status), WTERMSIG(status), (int)seen_sigchld);

    if (child != pid || !WIFSIGNALED(status) || WTERMSIG(status) != SIGINT) {
        printf("[FAIL] default SIGINT did not terminate child as expected\n");
        return 10;
    }
    if (seen_sigchld != SIGCHLD) {
        printf("[FAIL] SIGCHLD handler did not run\n");
        return 11;
    }

    printf("[OK] default SIGINT terminated the child and SIGCHLD reached parent\n");
    printf("=== SIGNAL TEST COMPLETE ===\n");
    return 0;
}
