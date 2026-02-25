#include "syscalls.h"

int main() {
    print("=========================\n");
    print("  Hello from ELF binary!\n");
    print("  Running in Ring 3\n");
    print("=========================\n");

    uint32_t pid = sys_getpid();
    char buf[] = "  PID: X\n";
    buf[7] = '0' + (pid % 10);
    sys_print(buf, 9);

    print("\n  Counting: ");
    for (int i = 0; i < 5; i++) {
        char c[] = "X ";
        c[0] = '0' + i;
        sys_print(c, 2);
        sys_sleep(500);
    }
    print("\n  Done!\n");

    return 0;
}
