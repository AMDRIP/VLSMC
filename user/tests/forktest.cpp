#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>

int main() {
    printf("=== FORK TEST ===\n");

    int pid = fork();

    if (pid < 0) {
        printf("[FAIL] fork() returned %d\n", pid);
        return 1;
    }

    if (pid == 0) {
        printf("[CHILD] I am the child process!\n");
        printf("[CHILD] My PID = %d\n", (int)syscall(SYS_GETPID));
        exit(42);
    } else {
        printf("[PARENT] fork() returned child PID = %d\n", pid);
        int status = 0;
        int child = wait(&status);
        printf("[PARENT] Child %d exited with code %d\n", child, status);
    }

    printf("=== FORK TEST COMPLETE ===\n");
    return 0;
}
