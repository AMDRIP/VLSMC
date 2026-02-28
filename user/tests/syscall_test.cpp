#include <sys/syscall.h>
#include <errno.h>

void print(const char* str) {
    int len = 0;
    while (str[len]) len++;
    syscall(SYS_PRINT, (long)str, (long)len);
}

int main() {
    print("Libc Syscall Test Starting...\n");

    long pid = syscall(SYS_GETPID);
    if (pid >= 0) {
        print("SYS_GETPID: SUCCESS\n");
    }

    print("Testing errno with invalid syscall...\n");
    long err = syscall(999); // Invalid syscall number
    if (err == -1 && errno != 0) {
        print("errno error handling: SUCCESS\n");
    } else {
        print("errno error handling: FAILED\n");
    }

    print("Libc Syscall Test Done.\n");
    return 0;
}
