#include "kernel/signal.h"
#include "kernel/syscall_gate.h"
#include "kernel/task_scheduler.h"
#include "kernel/vmm.h"

namespace re36 {
namespace Signal {

static int foreground_pgid = -1;

static bool valid_signal(int sig) {
    return sig > 0 && sig < MAX_SIGNALS;
}

static uint32_t signal_bit(int sig) {
    return 1u << sig;
}

static bool signal_can_be_caught_or_ignored(int sig) {
    return sig != KERNEL_SIGKILL;
}

static bool default_action_ignores(int sig) {
    return sig == KERNEL_SIGCHLD;
}

static bool signal_is_blocked(const Thread& thread, int sig) {
    if (sig == KERNEL_SIGKILL) return false;
    return (thread.signal_mask & signal_bit(sig)) != 0;
}

static bool valid_live_thread(int tid) {
    if (tid < 0 || tid >= MAX_THREADS) return false;
    ThreadState state = threads[tid].state;
    return state != ThreadState::Unused &&
           state != ThreadState::Terminated &&
           state != ThreadState::Zombie;
}

static bool user_ptr_ok(uint32_t ptr) {
    return ptr >= USER_SPACE_START && ptr < USER_SPACE_END;
}

static bool user_write_range_ok(uint32_t start, uint32_t size) {
    if (size == 0) return true;
    uint32_t end = start + size - 1;
    if (start < USER_SPACE_START || end >= USER_SPACE_END || end < start) return false;
    return VMM::get_physical(start) != 0 && VMM::get_physical(end) != 0;
}

static bool interrupted_user_mode(const Registers* regs) {
    return regs && ((regs->cs & 3) == 3);
}

static void save_context(const Registers* regs, SignalSavedContext& out) {
    out.ds = regs->ds;
    out.edi = regs->edi;
    out.esi = regs->esi;
    out.ebp = regs->ebp;
    out.esp = regs->esp;
    out.ebx = regs->ebx;
    out.edx = regs->edx;
    out.ecx = regs->ecx;
    out.eax = regs->eax;
    out.int_no = regs->int_no;
    out.err_code = regs->err_code;
    out.eip = regs->eip;
    out.cs = regs->cs;
    out.eflags = regs->eflags;
    out.useresp = regs->useresp;
    out.ss = regs->ss;
}

static void restore_context(Registers* regs, const SignalSavedContext& in) {
    regs->ds = in.ds;
    regs->edi = in.edi;
    regs->esi = in.esi;
    regs->ebp = in.ebp;
    regs->esp = in.esp;
    regs->ebx = in.ebx;
    regs->edx = in.edx;
    regs->ecx = in.ecx;
    regs->eax = in.eax;
    regs->int_no = in.int_no;
    regs->err_code = in.err_code;
    regs->eip = in.eip;
    regs->cs = in.cs;
    regs->eflags = in.eflags;
    regs->useresp = in.useresp;
    regs->ss = in.ss;
}

void init_thread(Thread& thread) {
    for (int i = 0; i < MAX_SIGNALS; i++) {
        thread.signal_handlers[i] = KERNEL_SIG_DFL;
        thread.signal_masks[i] = 0;
        thread.signal_flags[i] = 0;
    }
    thread.signal_trampoline = 0;
    thread.pending_signals = 0;
    thread.signal_mask = 0;
    thread.saved_signal_mask = 0;
    thread.signal_active = false;
    thread.active_signal = 0;

    uint32_t* ctx = (uint32_t*)&thread.signal_saved_context;
    for (uint32_t i = 0; i < sizeof(SignalSavedContext) / sizeof(uint32_t); i++) {
        ctx[i] = 0;
    }
}

void reset_for_exec(Thread& thread) {
    for (int i = 0; i < MAX_SIGNALS; i++) {
        if (thread.signal_handlers[i] != KERNEL_SIG_IGN) {
            thread.signal_handlers[i] = KERNEL_SIG_DFL;
        }
        thread.signal_masks[i] = 0;
        thread.signal_flags[i] = 0;
    }
    thread.signal_trampoline = 0;
    thread.pending_signals = 0;
    thread.signal_mask = 0;
    thread.saved_signal_mask = 0;
    thread.signal_active = false;
    thread.active_signal = 0;
}

void copy_for_fork(Thread& child, const Thread& parent) {
    for (int i = 0; i < MAX_SIGNALS; i++) {
        child.signal_handlers[i] = parent.signal_handlers[i];
        child.signal_masks[i] = parent.signal_masks[i];
        child.signal_flags[i] = parent.signal_flags[i];
    }
    child.signal_trampoline = parent.signal_trampoline;
    child.pending_signals = 0;
    child.signal_mask = parent.signal_mask;
    child.saved_signal_mask = 0;
    child.signal_active = false;
    child.active_signal = 0;
}

uint32_t set_handler(int tid, int sig, uint32_t handler, uint32_t trampoline) {
    KernelSigaction action;
    action.handler = handler;
    action.mask = 0;
    action.flags = 0;

    KernelSigaction old_action;
    if (set_action(tid, sig, action, trampoline, &old_action) == (uint32_t)-1) {
        return KERNEL_SIG_ERR;
    }
    return old_action.handler;
}

uint32_t set_action(int tid, int sig, const KernelSigaction& action,
                    uint32_t trampoline, KernelSigaction* old_action) {
    if (!valid_live_thread(tid) || !valid_signal(sig) ||
        !signal_can_be_caught_or_ignored(sig)) {
        return (uint32_t)-1;
    }

    if (action.handler != KERNEL_SIG_DFL &&
        action.handler != KERNEL_SIG_IGN &&
        !user_ptr_ok(action.handler)) {
        return (uint32_t)-1;
    }
    if (action.handler != KERNEL_SIG_DFL &&
        action.handler != KERNEL_SIG_IGN &&
        !user_ptr_ok(trampoline)) {
        return (uint32_t)-1;
    }

    Thread& target = threads[tid];
    if (old_action) {
        old_action->handler = target.signal_handlers[sig];
        old_action->mask = target.signal_masks[sig];
        old_action->flags = target.signal_flags[sig];
    }

    target.signal_handlers[sig] = action.handler;
    target.signal_masks[sig] = action.mask & ~(signal_bit(KERNEL_SIGKILL));
    target.signal_flags[sig] = action.flags;
    if (trampoline) {
        target.signal_trampoline = trampoline;
    }

    return 0;
}

uint32_t set_mask(int tid, int how, uint32_t set, uint32_t* old_set) {
    if (!valid_live_thread(tid)) return (uint32_t)-1;

    Thread& target = threads[tid];
    if (old_set) {
        *old_set = target.signal_mask;
    }

    set &= ~(signal_bit(KERNEL_SIGKILL));
    if (how == KERNEL_SIG_BLOCK) {
        target.signal_mask |= set;
    } else if (how == KERNEL_SIG_UNBLOCK) {
        target.signal_mask &= ~set;
    } else if (how == KERNEL_SIG_SETMASK) {
        target.signal_mask = set;
    } else {
        return (uint32_t)-1;
    }

    return 0;
}

uint32_t send(int tid, int sig) {
    if (!valid_live_thread(tid)) {
        return (uint32_t)-1;
    }

    if (sig == 0) {
        return 0;
    }
    if (!valid_signal(sig)) {
        return (uint32_t)-1;
    }

    Thread& target = threads[tid];
    target.pending_signals |= signal_bit(sig);

    if (target.state == ThreadState::Sleeping || target.state == ThreadState::Blocked) {
        target.state = ThreadState::Ready;
        target.blocked_channel_id = -1;
    }

    return 0;
}

uint32_t send_process_group(int pgid, int sig) {
    if (pgid < 0) return (uint32_t)-1;

    bool delivered = false;
    for (int i = 1; i < MAX_THREADS; i++) {
        if (!valid_live_thread(i)) continue;
        if (threads[i].process_group_id != pgid) continue;
        if (threads[i].is_driver) continue;
        if (send(i, sig) == 0) delivered = true;
    }

    return delivered ? 0 : (uint32_t)-1;
}

uint32_t send_for_kill(int pid, int sig) {
    if (pid > 0) {
        return send(pid, sig);
    }
    if (pid == 0) {
        if (current_tid < 0 || current_tid >= MAX_THREADS) return (uint32_t)-1;
        return send_process_group(threads[current_tid].process_group_id, sig);
    }
    if (pid == -1) {
        bool delivered = false;
        for (int i = 1; i < MAX_THREADS; i++) {
            if (!valid_live_thread(i) || threads[i].is_driver) continue;
            if (send(i, sig) == 0) delivered = true;
        }
        return delivered ? 0 : (uint32_t)-1;
    }
    return send_process_group(-pid, sig);
}

void notify_parent_of_exit(int parent_tid) {
    if (valid_live_thread(parent_tid)) {
        send(parent_tid, KERNEL_SIGCHLD);
    }
}

bool set_process_group(int tid, int pgid) {
    if (tid == 0) tid = current_tid;
    if (!valid_live_thread(tid)) return false;
    if (pgid == 0) pgid = tid;
    if (pgid < 0 || pgid >= MAX_THREADS) return false;
    if (threads[pgid].state == ThreadState::Unused && pgid != tid) return false;
    threads[tid].process_group_id = pgid;
    return true;
}

void set_foreground_process_group(int pgid) {
    foreground_pgid = pgid;
}

int get_foreground_process_group() {
    return foreground_pgid;
}

void send_sigint_from_keyboard() {
    if (foreground_pgid >= 0 && send_process_group(foreground_pgid, KERNEL_SIGINT) == 0) {
        return;
    }

    int target = -1;
    if (current_tid > 0 && valid_live_thread(current_tid) &&
               threads[current_tid].page_directory_phys != (uint32_t*)VMM::kernel_directory_phys_) {
        target = current_tid;
    } else {
        for (int i = 1; i < MAX_THREADS; i++) {
            if (valid_live_thread(i) &&
                !threads[i].is_driver &&
                threads[i].page_directory_phys != (uint32_t*)VMM::kernel_directory_phys_) {
                target = i;
                break;
            }
        }
    }

    if (target >= 0) {
        send(target, KERNEL_SIGINT);
    }
}

bool deliver_pending(Registers* regs) {
    if (!interrupted_user_mode(regs)) return false;
    if (current_tid < 0 || current_tid >= MAX_THREADS) return false;

    Thread& current = threads[current_tid];
    if (current.signal_active) return false;

    for (int sig = 1; sig < MAX_SIGNALS; sig++) {
        uint32_t bit = signal_bit(sig);
        if ((current.pending_signals & bit) == 0) continue;
        if (signal_is_blocked(current, sig)) continue;

        uint32_t handler = current.signal_handlers[sig];
        if (handler == KERNEL_SIG_IGN && sig != KERNEL_SIGKILL) {
            current.pending_signals &= ~bit;
            continue;
        }

        if (handler == KERNEL_SIG_DFL && default_action_ignores(sig)) {
            current.pending_signals &= ~bit;
            continue;
        }

        if (handler == KERNEL_SIG_DFL || sig == KERNEL_SIGKILL) {
            current.pending_signals &= ~bit;
            exit_current_thread_signal(sig);
            return true;
        }

        uint32_t new_useresp = regs->useresp - 8;
        if (!user_ptr_ok(handler) ||
            !user_ptr_ok(current.signal_trampoline) ||
            !user_write_range_ok(new_useresp, 8)) {
            current.pending_signals &= ~bit;
            exit_current_thread_signal(sig);
            return true;
        }

        save_context(regs, current.signal_saved_context);
        current.saved_signal_mask = current.signal_mask;
        current.signal_mask |= current.signal_masks[sig] | bit;
        current.signal_mask &= ~(signal_bit(KERNEL_SIGKILL));
        current.signal_active = true;
        current.active_signal = (uint32_t)sig;
        current.pending_signals &= ~bit;

        uint32_t* frame = (uint32_t*)new_useresp;
        frame[0] = current.signal_trampoline;
        frame[1] = (uint32_t)sig;

        regs->useresp = new_useresp;
        regs->eip = handler;
        regs->eflags |= 0x200;
        return true;
    }

    return false;
}

uint32_t sigreturn(Registers* regs) {
    if (!regs || current_tid < 0 || current_tid >= MAX_THREADS) return (uint32_t)-1;

    Thread& current = threads[current_tid];
    if (!current.signal_active) {
        return (uint32_t)-1;
    }

    restore_context(regs, current.signal_saved_context);
    current.signal_mask = current.saved_signal_mask;
    current.signal_active = false;
    current.active_signal = 0;
    return regs->eax;
}

} // namespace Signal
} // namespace re36
