#include "kernel/shell_history.h"
#include "libc.h"

namespace re36 {

char ShellHistory::entries_[SHELL_HISTORY_SIZE][SHELL_MAX_CMD_LEN];
int ShellHistory::head_ = 0;
int ShellHistory::count_ = 0;
int ShellHistory::cursor_ = -1;

void ShellHistory::init() {
    head_ = 0;
    count_ = 0;
    cursor_ = -1;
    for (int i = 0; i < SHELL_HISTORY_SIZE; i++)
        entries_[i][0] = '\0';
}

void ShellHistory::add(const char* cmd) {
    if (!cmd || !cmd[0]) return;

    if (count_ > 0) {
        int last = (head_ - 1 + SHELL_HISTORY_SIZE) % SHELL_HISTORY_SIZE;
        bool same = true;
        for (int i = 0; entries_[last][i] || cmd[i]; i++) {
            if (entries_[last][i] != cmd[i]) { same = false; break; }
        }
        if (same) return;
    }

    int i = 0;
    while (cmd[i] && i < SHELL_MAX_CMD_LEN - 1) {
        entries_[head_][i] = cmd[i];
        i++;
    }
    entries_[head_][i] = '\0';

    head_ = (head_ + 1) % SHELL_HISTORY_SIZE;
    if (count_ < SHELL_HISTORY_SIZE) count_++;
    cursor_ = -1;
}

const char* ShellHistory::navigate_up() {
    if (count_ == 0) return nullptr;

    if (cursor_ == -1) {
        cursor_ = 0;
    } else if (cursor_ < count_ - 1) {
        cursor_++;
    } else {
        return entries_[(head_ - 1 - cursor_ + SHELL_HISTORY_SIZE) % SHELL_HISTORY_SIZE];
    }

    int idx = (head_ - 1 - cursor_ + SHELL_HISTORY_SIZE) % SHELL_HISTORY_SIZE;
    return entries_[idx];
}

const char* ShellHistory::navigate_down() {
    if (cursor_ <= 0) {
        cursor_ = -1;
        return "";
    }

    cursor_--;
    int idx = (head_ - 1 - cursor_ + SHELL_HISTORY_SIZE) % SHELL_HISTORY_SIZE;
    return entries_[idx];
}

void ShellHistory::reset_cursor() {
    cursor_ = -1;
}

} // namespace re36
