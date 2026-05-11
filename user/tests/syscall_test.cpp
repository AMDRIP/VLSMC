#include <sys/syscall.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

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

    const char* msg = "SYS_WRITE stdout: SUCCESS\n";
    if (write(STDOUT_FILENO, msg, 26) == 26) {
        print("POSIX write(): SUCCESS\n");
    } else {
        print("POSIX write(): FAILED\n");
    }

    struct stat st;
    if (stat("/HELLO.TXT", &st) == 0 && st.st_size > 0) {
        print("POSIX stat(): SUCCESS\n");
    } else {
        print("POSIX stat(): FAILED\n");
    }

    print("Libc Syscall Test Done.\n");
    return 0;
}
