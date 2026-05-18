#pragma once

#include <stdint.h>
#include "kernel/idt.h"
#include "kernel/thread.h"

namespace re36 {

#define KERNEL_SIGHUP   1
#define KERNEL_SIGINT   2
#define KERNEL_SIGQUIT  3
#define KERNEL_SIGILL   4
#define KERNEL_SIGTRAP  5
#define KERNEL_SIGABRT  6
#define KERNEL_SIGBUS   7
#define KERNEL_SIGFPE   8
#define KERNEL_SIGKILL  9
#define KERNEL_SIGUSR1 10
#define KERNEL_SIGSEGV 11
#define KERNEL_SIGUSR2 12
#define KERNEL_SIGPIPE 13
#define KERNEL_SIGALRM 14
#define KERNEL_SIGTERM 15
#define KERNEL_SIGCHLD 17

#define KERNEL_SIG_DFL 0u
#define KERNEL_SIG_IGN 1u
#define KERNEL_SIG_ERR 0xFFFFFFFFu

#define KERNEL_SIG_BLOCK   0
#define KERNEL_SIG_UNBLOCK 1
#define KERNEL_SIG_SETMASK 2

struct KernelSigaction {
    uint32_t handler;
    uint32_t mask;
    uint32_t flags;
};

namespace Signal {

void init_thread(Thread& thread);
void reset_for_exec(Thread& thread);
void copy_for_fork(Thread& child, const Thread& parent);

uint32_t set_handler(int tid, int sig, uint32_t handler, uint32_t trampoline);
uint32_t set_action(int tid, int sig, const KernelSigaction& action,
                    uint32_t trampoline, KernelSigaction* old_action);
uint32_t set_mask(int tid, int how, uint32_t set, uint32_t* old_set);
uint32_t send(int tid, int sig);
uint32_t send_process_group(int pgid, int sig);
uint32_t send_for_kill(int pid, int sig);
void notify_parent_of_exit(int parent_tid);
void send_sigint_from_keyboard();

bool deliver_pending(Registers* regs);
uint32_t sigreturn(Registers* regs);

bool set_process_group(int tid, int pgid);
void set_foreground_process_group(int pgid);
int get_foreground_process_group();

} // namespace Signal

} // namespace re36
