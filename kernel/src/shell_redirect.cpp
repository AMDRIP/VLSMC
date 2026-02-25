#include "kernel/shell_redirect.h"
#include "libc.h"

namespace re36 {

char ShellRedirect::buffer_[SHELL_OUTPUT_BUF_SIZE];
int ShellRedirect::pos_ = 0;
bool ShellRedirect::capturing_ = false;

bool ShellRedirect::parse(const char* cmdline, char* cmd_out, int cmd_max, char* redir_file, int file_max) {
    redir_file[0] = '\0';
    int i = 0;
    int ci = 0;

    while (cmdline[i] && cmdline[i] != '>') {
        if (ci < cmd_max - 1) cmd_out[ci++] = cmdline[i];
        i++;
    }

    while (ci > 0 && cmd_out[ci - 1] == ' ') ci--;
    cmd_out[ci] = '\0';

    if (cmdline[i] == '>') {
        i++;
        while (cmdline[i] == ' ') i++;
        int fi = 0;
        while (cmdline[i] && fi < file_max - 1) {
            redir_file[fi++] = cmdline[i++];
        }
        while (fi > 0 && redir_file[fi - 1] == ' ') fi--;
        redir_file[fi] = '\0';
        return fi > 0;
    }

    return false;
}

bool ShellRedirect::parse_pipe(const char* cmdline, char* cmd1, int max1, char* cmd2, int max2) {
    int i = 0;
    int ci = 0;

    while (cmdline[i] && cmdline[i] != '|') {
        if (ci < max1 - 1) cmd1[ci++] = cmdline[i];
        i++;
    }
    while (ci > 0 && cmd1[ci - 1] == ' ') ci--;
    cmd1[ci] = '\0';

    if (cmdline[i] != '|') return false;
    i++;
    while (cmdline[i] == ' ') i++;

    ci = 0;
    while (cmdline[i]) {
        if (ci < max2 - 1) cmd2[ci++] = cmdline[i];
        i++;
    }
    while (ci > 0 && cmd2[ci - 1] == ' ') ci--;
    cmd2[ci] = '\0';

    return ci > 0;
}

void ShellRedirect::begin_capture() {
    pos_ = 0;
    buffer_[0] = '\0';
    capturing_ = true;
}

void ShellRedirect::end_capture() {
    capturing_ = false;
    buffer_[pos_] = '\0';
}

bool ShellRedirect::is_capturing() {
    return capturing_;
}

const char* ShellRedirect::get_buffer() {
    return buffer_;
}

int ShellRedirect::get_length() {
    return pos_;
}

void ShellRedirect::append(const char* str, int len) {
    for (int i = 0; i < len && pos_ < SHELL_OUTPUT_BUF_SIZE - 1; i++) {
        buffer_[pos_++] = str[i];
    }
    buffer_[pos_] = '\0';
}

} // namespace re36
