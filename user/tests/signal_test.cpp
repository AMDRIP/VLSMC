#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

static volatile sig_atomic_t seen_sigint = 0;

static void on_sigint(int sig) {
    seen_sigint = sig;
    printf("[handler] SIGINT handler ran with sig=%d\n", sig);
}

int main() {
    printf("=== SIGNAL TEST ===\n");

    if (signal(SIGINT, on_sigint) == SIG_ERR) {
        printf("[FAIL] signal(SIGINT) returned SIG_ERR\n");
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

    int pid = fork();
    if (pid < 0) {
        printf("[FAIL] fork failed\n");
        return 4;
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
    int code = WEXITSTATUS(status);
    printf("[parent] child=%d status=0x%x exit_code=%d\n", child, status, code);

    if (child != pid || code != 128 + SIGINT) {
        printf("[FAIL] default SIGINT did not terminate child as expected\n");
        return 6;
    }

    printf("[OK] default SIGINT terminated the child\n");
    printf("=== SIGNAL TEST COMPLETE ===\n");
    return 0;
}
