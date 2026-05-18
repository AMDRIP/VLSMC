#include "kernel/signal.h"
#include "kernel/syscall_gate.h"
#include "kernel/task_scheduler.h"
#include "kernel/vmm.h"

namespace re36 {
namespace Signal {

static int foreground_tid = -1;

static bool valid_signal(int sig) {
    return sig > 0 && sig < MAX_SIGNALS;
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
    }
    thread.signal_trampoline = 0;
    thread.pending_signals = 0;
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
    }
    thread.signal_trampoline = 0;
    thread.pending_signals = 0;
    thread.signal_active = false;
    thread.active_signal = 0;
}

void copy_for_fork(Thread& child, const Thread& parent) {
    for (int i = 0; i < MAX_SIGNALS; i++) {
        child.signal_handlers[i] = parent.signal_handlers[i];
    }
    child.signal_trampoline = parent.signal_trampoline;
    child.pending_signals = 0;
    child.signal_active = parent.signal_active;
    child.active_signal = parent.active_signal;
    child.signal_saved_context = parent.signal_saved_context;
}

uint32_t set_handler(int tid, int sig, uint32_t handler, uint32_t trampoline) {
    if (!valid_live_thread(tid) || !valid_signal(sig) || sig == KERNEL_SIGKILL) {
        return KERNEL_SIG_ERR;
    }

    if (handler != KERNEL_SIG_DFL && handler != KERNEL_SIG_IGN && !user_ptr_ok(handler)) {
        return KERNEL_SIG_ERR;
    }
    if (handler != KERNEL_SIG_DFL && handler != KERNEL_SIG_IGN && !user_ptr_ok(trampoline)) {
        return KERNEL_SIG_ERR;
    }

    Thread& target = threads[tid];
    uint32_t old = target.signal_handlers[sig];
    target.signal_handlers[sig] = handler;
    if (trampoline) {
        target.signal_trampoline = trampoline;
    }
    return old;
}

uint32_t send(int tid, int sig) {
    if (tid == 0) {
        tid = current_tid;
    }
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
    target.pending_signals |= (1u << sig);

    if (target.state == ThreadState::Sleeping || target.state == ThreadState::Blocked) {
        target.state = ThreadState::Ready;
        target.blocked_channel_id = -1;
    }

    return 0;
}

void set_foreground_tid(int tid) {
    foreground_tid = tid;
}

int get_foreground_tid() {
    return foreground_tid;
}

void send_sigint_from_keyboard() {
    int target = -1;
    if (valid_live_thread(foreground_tid)) {
        target = foreground_tid;
    } else if (current_tid > 0 && valid_live_thread(current_tid) &&
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
        uint32_t bit = 1u << sig;
        if ((current.pending_signals & bit) == 0) continue;

        uint32_t handler = current.signal_handlers[sig];
        if (handler == KERNEL_SIG_IGN && sig != KERNEL_SIGKILL) {
            current.pending_signals &= ~bit;
            continue;
        }

        if (handler == KERNEL_SIG_DFL || sig == KERNEL_SIGKILL) {
            current.pending_signals &= ~bit;
            exit_current_thread(128 + sig);
            return true;
        }

        uint32_t new_useresp = regs->useresp - 8;
        if (!user_ptr_ok(handler) ||
            !user_ptr_ok(current.signal_trampoline) ||
            !user_write_range_ok(new_useresp, 8)) {
            current.pending_signals &= ~bit;
            exit_current_thread(128 + sig);
            return true;
        }

        save_context(regs, current.signal_saved_context);
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
    current.signal_active = false;
    current.active_signal = 0;
    return regs->eax;
}

} // namespace Signal
} // namespace re36
