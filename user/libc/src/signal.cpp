#include <signal.h>

void (*signal(int sig, void (*func)(int)))(int) {
    return SIG_ERR; // Stub
}

int raise(int sig) {
    return -1;
}

int kill(int pid, int sig) {
    return -1;
}
