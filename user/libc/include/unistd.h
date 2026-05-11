#pragma once

#include <sys/types.h>
#include <stddef.h>

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#ifdef __cplusplus
extern "C" {
#endif

ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int close(int fd);
off_t lseek(int fd, off_t offset, int whence);

int unlink(const char *pathname);
int link(const char *oldpath, const char *newpath);
int symlink(const char *target, const char *linkpath);
ssize_t readlink(const char *pathname, char *buf, size_t bufsiz);
pid_t getpid(void);
pid_t fork(void);
int execvp(const char *file, char *const argv[]);
int execve(const char *pathname, char *const argv[], char *const envp[]);
int exec(const char *path);

unsigned int sleep(unsigned int seconds);
int usleep(unsigned int usec);

void _exit(int status);

#ifdef __cplusplus
}
#endif
