#pragma once

#include <stdint.h>

namespace re36 {

#define SHELL_OUTPUT_BUF_SIZE 4096

class ShellRedirect {
public:
    static bool parse(const char* cmdline, char* cmd_out, int cmd_max, char* redir_file, int file_max);
    static bool parse_pipe(const char* cmdline, char* cmd1, int max1, char* cmd2, int max2);
    
    static void begin_capture();
    static void end_capture();
    static bool is_capturing();
    static const char* get_buffer();
    static int get_length();
    static void append(const char* str, int len);

private:
    static char buffer_[SHELL_OUTPUT_BUF_SIZE];
    static int pos_;
    static bool capturing_;
};

} // namespace re36
