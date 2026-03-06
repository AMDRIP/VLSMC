#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "../../syscalls.h"

// unistd.h overrides
ssize_t read(int fd, void *buf, size_t count) { return syscall3(SYS_FREAD, fd, (uint32_t)buf, count); }
ssize_t write(int fd, const void *buf, size_t count) { return syscall3(SYS_FWRITE, fd, (uint32_t)buf, count); }
int close(int fd) { return syscall1(SYS_FCLOSE, fd); }
off_t lseek(int fd, off_t offset, int whence) { return syscall3(SYS_FSEEK, fd, offset, whence); }
int unlink(const char *pathname) { return -1; }
pid_t getpid(void) { return syscall0(SYS_GETPID); }
pid_t fork(void) { return syscall0(SYS_FORK); }
int execve(const char *pathname, char *const argv[], char *const envp[]) {
    return syscall1(SYS_EXEC, (uint32_t)pathname);
}
int execvp(const char *file, char *const argv[]) { return execve(file, argv, nullptr); }
int exec(const char *path) {
    char *argv[] = { (char*)path, nullptr };
    return execve(path, argv, nullptr);
}
unsigned int sleep(unsigned int seconds) { sys_sleep(seconds * 1000); return 0; }
int usleep(unsigned int usec) { sys_sleep(usec / 1000); return 0; }
void _exit(int status) { sys_exit(); }

// fcntl.h overrides
int open(const char* path, int flags, ...) { return syscall2(SYS_FOPEN, (uint32_t)path, flags); }

// stat.h overrides
int stat(const char *path, struct stat *buf) { return -1; }
int fstat(int fd, struct stat *buf) { return -1; }
int mkdir(const char *path, mode_t mode) { return -1; }

// wait.h overrides
pid_t wait(int *wstatus) { return syscall1(SYS_WAIT, (uint32_t)wstatus); }
pid_t waitpid(pid_t pid, int *wstatus, int options) { return syscall1(SYS_WAIT, (uint32_t)wstatus); }
