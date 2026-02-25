#include "kernel/shell.h"
#include "kernel/shell_history.h"
#include "kernel/shell_autocomplete.h"
#include "kernel/shell_redirect.h"
#include "kernel/keyboard.h"
#include "kernel/fat16.h"
#include "kernel/ata.h"
#include "kernel/pmm.h"
#include "kernel/vmm.h"
#include "kernel/timer.h"
#include "kernel/rtc.h"
#include "kernel/task_scheduler.h"
#include "kernel/thread.h"
#include "kernel/elf_loader.h"
#include "kernel/usermode.h"
#include "libc.h"

namespace re36 {

static char input_buf[SHELL_MAX_CMD_LEN];
static int input_len = 0;
static int cursor_x = 0;

static volatile uint16_t* vga = (volatile uint16_t*)0xB8000;

static void clear_input_line() {
    int row = 24;
    for (int x = 2; x < 80; x++)
        vga[row * 80 + x] = (uint16_t)(' ') | (0x1F << 8);
}

static void redraw_input() {
    clear_input_line();
    printf("\r> %s", input_buf);
}

static void set_input(const char* str) {
    input_len = 0;
    while (str[input_len] && input_len < SHELL_MAX_CMD_LEN - 1) {
        input_buf[input_len] = str[input_len];
        input_len++;
    }
    input_buf[input_len] = '\0';
    redraw_input();
}

static bool str_eq(const char* a, const char* b) {
    while (*a && *b) { if (*a++ != *b++) return false; }
    return *a == *b;
}

static bool str_starts(const char* str, const char* prefix, int len) {
    for (int i = 0; i < len; i++) {
        if (str[i] != prefix[i]) return false;
    }
    return true;
}

static const char* str_after(const char* str, int skip) {
    return str + skip;
}

static void exec_command(const char* cmd) {
    if (!cmd || !cmd[0]) return;

    if (str_eq(cmd, "hello")) {
        printf("Hello to you too, Kernel Hacker!\n");
    } else if (str_eq(cmd, "clear")) {
        for (int i = 0; i < 80 * 25; i++)
            vga[i] = (uint16_t)(' ') | (0x1F << 8);
        printf("\n");
    } else if (str_eq(cmd, "ps")) {
        TaskScheduler::print_threads();
    } else if (str_eq(cmd, "ticks")) {
        printf("Timer ticks: %u\n", Timer::get_ticks());
    } else if (str_eq(cmd, "meminfo")) {
        printf("Free RAM: %u KB\n", PhysicalMemoryManager::get_free_memory() / 1024);
        printf("Used RAM: %u KB\n", PhysicalMemoryManager::get_used_memory() / 1024);
        printf("Paging: Enabled (CR3 = 0x%x)\n", (uint32_t)VMM::get_current_directory());
    } else if (str_eq(cmd, "date")) {
        DateTime dt;
        RTC::read(dt);
        printf("%d-%d-%d %d:%d:%d\n", dt.year, dt.month, dt.day, dt.hours, dt.minutes, dt.seconds);
    } else if (str_eq(cmd, "syscall")) {
        printf("Testing int 0x80 (SYS_GETPID)...\n");
        uint32_t tid;
        asm volatile("mov $5, %%eax; int $0x80; mov %%eax, %0" : "=r"(tid) :: "eax");
        printf("Syscall returned TID = %d\n", tid);
    } else if (str_eq(cmd, "help")) {
        printf("File: ls, cat, write, rm, stat, hexdump, exec\n");
        printf("System: ps, ticks, meminfo, date, syscall, ring3, clear, help\n");
        printf("Shell: Tab=autocomplete, Up/Down=history, >=redirect, |=pipe\n");
    } else if (str_eq(cmd, "ring3")) {
        printf("Launching Ring 3 user process...\n");
        void (*entry)() = []() { enter_usermode(); };
        thread_create("user0", entry, 10);
    } else if (str_eq(cmd, "ls")) {
        Fat16::list_root();
    } else if (str_starts(cmd, "exec ", 5)) {
        elf_exec(str_after(cmd, 5));
    } else if (str_starts(cmd, "cat ", 4)) {
        static uint8_t file_buf[4096];
        int bytes = Fat16::read_file(str_after(cmd, 4), file_buf, sizeof(file_buf) - 1);
        if (bytes < 0) {
            printf("File not found: %s\n", str_after(cmd, 4));
        } else {
            file_buf[bytes] = '\0';
            printf("%s\n", (const char*)file_buf);
        }
    } else if (str_starts(cmd, "write ", 6)) {
        const char* args = str_after(cmd, 6);
        const char* space = args;
        while (*space && *space != ' ') space++;
        if (*space == ' ') {
            int name_len = space - args;
            char fname[32];
            for (int k = 0; k < name_len && k < 31; k++) fname[k] = args[k];
            fname[name_len < 31 ? name_len : 31] = '\0';
            const char* content = space + 1;
            int clen = 0;
            while (content[clen]) clen++;
            if (Fat16::write_file(fname, (const uint8_t*)content, clen))
                printf("Written %d bytes to %s\n", clen, fname);
            else
                printf("Write failed!\n");
        } else {
            printf("Usage: write <filename> <content>\n");
        }
    } else if (str_starts(cmd, "rm ", 3)) {
        if (Fat16::delete_file(str_after(cmd, 3)))
            printf("Deleted: %s\n", str_after(cmd, 3));
    } else if (str_starts(cmd, "stat ", 5)) {
        Fat16::stat_file(str_after(cmd, 5));
    } else if (str_starts(cmd, "hexdump ", 8)) {
        static uint8_t hbuf[256];
        int bytes = Fat16::read_file(str_after(cmd, 8), hbuf, sizeof(hbuf));
        if (bytes < 0) {
            printf("File not found: %s\n", str_after(cmd, 8));
        } else {
            for (int off = 0; off < bytes; off += 16) {
                printf("%x: ", off);
                for (int j = 0; j < 16 && off + j < bytes; j++)
                    printf("%x ", hbuf[off + j]);
                printf(" ");
                for (int j = 0; j < 16 && off + j < bytes; j++) {
                    char ch = hbuf[off + j];
                    printf("%c", (ch >= 32 && ch <= 126) ? ch : '.');
                }
                printf("\n");
            }
        }
    } else {
        printf("Unknown command: %s\n", cmd);
    }
}

static void process_line(const char* line) {
    if (!line[0]) return;

    ShellHistory::add(line);

    char cmd1[SHELL_MAX_CMD_LEN];
    char cmd2[SHELL_MAX_CMD_LEN];
    char redir_file[64];

    if (ShellRedirect::parse_pipe(line, cmd1, SHELL_MAX_CMD_LEN, cmd2, SHELL_MAX_CMD_LEN)) {
        ShellRedirect::begin_capture();
        exec_command(cmd1);
        ShellRedirect::end_capture();

        const char* pipe_data = ShellRedirect::get_buffer();

        if (str_starts(cmd2, "write ", 6)) {
            const char* fname = str_after(cmd2, 6);
            int plen = ShellRedirect::get_length();
            if (Fat16::write_file(fname, (const uint8_t*)pipe_data, plen))
                printf("Piped %d bytes to %s\n", plen, fname);
            else
                printf("Pipe write failed!\n");
        } else {
            printf("%s", pipe_data);
            exec_command(cmd2);
        }
        return;
    }

    if (ShellRedirect::parse(line, cmd1, SHELL_MAX_CMD_LEN, redir_file, 64)) {
        ShellRedirect::begin_capture();
        exec_command(cmd1);
        ShellRedirect::end_capture();

        int len = ShellRedirect::get_length();
        if (Fat16::write_file(redir_file, (const uint8_t*)ShellRedirect::get_buffer(), len))
            printf("Redirected %d bytes to %s\n", len, redir_file);
        else
            printf("Redirect failed!\n");
        return;
    }

    exec_command(line);
}

void shell_main() {
    ShellHistory::init();

    printf("RAND Elecorner 36 Shell v2.0\n");
    printf("Type 'help' for commands. Tab=complete, Up/Down=history\n\n");
    printf("> ");

    input_len = 0;
    input_buf[0] = '\0';

    while (true) {
        char c = getchar();

        if (c == '\n') {
            printf("\n");
            process_line(input_buf);
            ShellHistory::reset_cursor();
            input_len = 0;
            input_buf[0] = '\0';
            printf("> ");
        } else if (c == '\b') {
            if (input_len > 0) {
                input_len--;
                input_buf[input_len] = '\0';
            }
        } else if (c == '\t') {
            const char* completed = ShellAutocomplete::complete(input_buf);
            if (completed) {
                set_input(completed);
            }
        } else if (c == (char)0x80) {
            const char* prev = ShellHistory::navigate_up();
            if (prev) set_input(prev);
        } else if (c == (char)0x81) {
            const char* next = ShellHistory::navigate_down();
            if (next) set_input(next);
        } else if (c >= 32 && c <= 126) {
            if (input_len < SHELL_MAX_CMD_LEN - 1) {
                input_buf[input_len++] = c;
                input_buf[input_len] = '\0';
            }
        }
    }
}

} // namespace re36
