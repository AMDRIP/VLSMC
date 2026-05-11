#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <errno.h>
#include <stdarg.h>

// unistd.h overrides
ssize_t read(int fd, void *buf, size_t count) { return (ssize_t)syscall(SYS_READ, fd, (long)buf, count); }
ssize_t write(int fd, const void *buf, size_t count) { return (ssize_t)syscall(SYS_WRITE, fd, (long)buf, count); }
int close(int fd) { return (int)syscall(SYS_CLOSE, fd); }
off_t lseek(int fd, off_t offset, int whence) { return (off_t)syscall(SYS_FSEEK, fd, offset, whence); }
int unlink(const char *pathname) { return (int)syscall(SYS_UNLINK, (long)pathname); }
pid_t getpid(void) { return (pid_t)syscall(SYS_GETPID); }
pid_t fork(void) { return (pid_t)syscall(SYS_FORK); }
int execve(const char *pathname, char *const argv[], char *const envp[]) {
    return (int)syscall(SYS_EXEC, (long)pathname, (long)argv, (long)envp);
}
int execvp(const char *file, char *const argv[]) { return execve(file, argv, nullptr); }
int exec(const char *path) {
    char *argv[] = { (char*)path, nullptr };
    return execve(path, argv, nullptr);
}
unsigned int sleep(unsigned int seconds) { syscall(SYS_SLEEP, seconds * 1000); return 0; }
int usleep(unsigned int usec) { syscall(SYS_SLEEP, usec / 1000); return 0; }
void _exit(int status) { syscall(SYS_EXIT, status); while (1) {} }

// fcntl.h overrides
int open(const char* path, int flags, ...) {
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list ap;
        va_start(ap, flags);
        mode = va_arg(ap, mode_t);
        va_end(ap);
    }
    return (int)syscall(SYS_OPEN, (long)path, flags, mode);
}

// stat.h overrides
int stat(const char *path, struct stat *buf) { return (int)syscall(SYS_STAT, (long)path, (long)buf); }
int fstat(int fd, struct stat *buf) { return (int)syscall(SYS_FSTAT, fd, (long)buf); }
int mkdir(const char *path, mode_t mode) { return (int)syscall(SYS_MKDIR, (long)path, mode); }

// wait.h overrides
pid_t wait(int *wstatus) { return (pid_t)syscall(SYS_WAIT, (long)wstatus); }
pid_t waitpid(pid_t pid, int *wstatus, int options) {
    return (pid_t)syscall(SYS_WAITPID, pid, (long)wstatus, options);
}
