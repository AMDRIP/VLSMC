#include "kernel/shell_redirect.h"
#include "libc.h"

namespace re36 {

char ShellRedirect::buffer_[SHELL_OUTPUT_BUF_SIZE];
int ShellRedirect::pos_ = 0;
bool ShellRedirect::capturing_ = false;
bool ShellRedirect::overflowed_ = false;

static bool is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static int str_len(const char* str) {
    int len = 0;
    while (str && str[len]) len++;
    return len;
}

static int find_unquoted(const char* str, char target) {
    bool in_single = false;
    bool in_double = false;

    for (int i = 0; str[i]; i++) {
        char c = str[i];
        if (c == '\\' && !in_single && str[i + 1]) {
            i++;
            continue;
        }
        if (c == '\'' && !in_double) {
            in_single = !in_single;
            continue;
        }
        if (c == '"' && !in_single) {
            in_double = !in_double;
            continue;
        }
        if (c == target && !in_single && !in_double) return i;
    }

    return -1;
}

static int copy_trimmed_range(const char* src, int start, int end, char* out, int max) {
    while (start < end && is_space(src[start])) start++;
    while (end > start && is_space(src[end - 1])) end--;

    int oi = 0;
    for (int i = start; i < end && oi < max - 1; i++) {
        out[oi++] = src[i];
    }
    out[oi] = '\0';
    return oi;
}

bool ShellRedirect::parse(const char* cmdline, char* cmd_out, int cmd_max, char* redir_file, int file_max, bool* append_out) {
    if (cmd_out && cmd_max > 0) cmd_out[0] = '\0';
    if (redir_file && file_max > 0) redir_file[0] = '\0';
    if (append_out) *append_out = false;
    if (!cmdline || !cmd_out || !redir_file || cmd_max <= 0 || file_max <= 0) return false;

    int redir = find_unquoted(cmdline, '>');
    if (redir < 0) return false;

    bool append = cmdline[redir + 1] == '>';
    int file_start = redir + (append ? 2 : 1);
    int len = str_len(cmdline);
    int cmd_len = copy_trimmed_range(cmdline, 0, redir, cmd_out, cmd_max);
    int file_len = copy_trimmed_range(cmdline, file_start, len, redir_file, file_max);
    if (append_out) *append_out = append;
    return cmd_len > 0 && file_len > 0;
}

bool ShellRedirect::parse_pipe(const char* cmdline, char* cmd1, int max1, char* cmd2, int max2) {
    if (cmd1 && max1 > 0) cmd1[0] = '\0';
    if (cmd2 && max2 > 0) cmd2[0] = '\0';
    if (!cmdline || !cmd1 || !cmd2 || max1 <= 0 || max2 <= 0) return false;

    int pipe = find_unquoted(cmdline, '|');
    if (pipe < 0) return false;

    int len = str_len(cmdline);
    int len1 = copy_trimmed_range(cmdline, 0, pipe, cmd1, max1);
    int len2 = copy_trimmed_range(cmdline, pipe + 1, len, cmd2, max2);
    return len1 > 0 && len2 > 0;
}

void ShellRedirect::begin_capture() {
    pos_ = 0;
    buffer_[0] = '\0';
    overflowed_ = false;
    capturing_ = true;
}

void ShellRedirect::end_capture() {
    capturing_ = false;
    buffer_[pos_] = '\0';
}

bool ShellRedirect::is_capturing() {
    return capturing_;
}

bool ShellRedirect::has_overflowed() {
    return overflowed_;
}

const char* ShellRedirect::get_buffer() {
    return buffer_;
}

int ShellRedirect::get_length() {
    return pos_;
}

void ShellRedirect::append(const char* str, int len) {
    if (!str || len <= 0) return;

    for (int i = 0; i < len; i++) {
        if (pos_ < SHELL_OUTPUT_BUF_SIZE - 1) {
            buffer_[pos_++] = str[i];
        } else {
            overflowed_ = true;
        }
    }
    buffer_[pos_] = '\0';
}

} // namespace re36
