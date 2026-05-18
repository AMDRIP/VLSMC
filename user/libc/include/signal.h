#pragma once

#define SIGHUP    1
#define SIGINT    2
#define SIGQUIT   3
#define SIGILL    4
#define SIGTRAP   5
#define SIGABRT   6
#define SIGIOT    6
#define SIGBUS    7
#define SIGFPE    8
#define SIGKILL   9
#define SIGUSR1  10
#define SIGSEGV  11
#define SIGUSR2  12
#define SIGPIPE  13
#define SIGALRM  14
#define SIGTERM  15

#define SIG_DFL ((void (*)(int))0)
#define SIG_IGN ((void (*)(int))1)
#define SIG_ERR ((void (*)(int))-1)

typedef int sig_atomic_t;
typedef void (*sighandler_t)(int);

#ifdef __cplusplus
extern "C" {
#endif

sighandler_t signal(int sig, sighandler_t func);
int raise(int sig);
int kill(int pid, int sig);

#ifdef __cplusplus
}
#endif
