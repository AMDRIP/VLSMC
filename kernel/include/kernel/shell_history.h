#pragma once

#include <stdint.h>

namespace re36 {

#define SHELL_HISTORY_SIZE 16
#define SHELL_MAX_CMD_LEN  128

class ShellHistory {
public:
    static void init();
    static void add(const char* cmd);
    static const char* navigate_up();
    static const char* navigate_down();
    static void reset_cursor();

private:
    static char entries_[SHELL_HISTORY_SIZE][SHELL_MAX_CMD_LEN];
    static int head_;
    static int count_;
    static int cursor_;
};

} // namespace re36
