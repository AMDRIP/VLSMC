#pragma once

#include <sys/types.h>

#define WNOHANG   1
#define WUNTRACED 2

#define WIFEXITED(status)   (((status) & 0x7f) == 0)
#define WEXITSTATUS(status) (((status) & 0xff00) >> 8)
#define WIFSIGNALED(status) (!WIFEXITED(status) && !WIFSTOPPED(status))
#define WTERMSIG(status)    ((status) & 0x7f)
#define WIFSTOPPED(status)  (((status) & 0xff) == 0x7f)
#define WSTOPSIG(status)    (((status) & 0xff00) >> 8)

#ifdef __cplusplus
extern "C" {
#endif

pid_t wait(int *wstatus);
pid_t waitpid(pid_t pid, int *wstatus, int options);

#ifdef __cplusplus
}
#endif
