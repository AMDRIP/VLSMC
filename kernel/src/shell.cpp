#include "kernel/shell.h"
#include "kernel/shell_history.h"
#include "kernel/shell_autocomplete.h"
#include "kernel/shell_redirect.h"
#include "kernel/keyboard.h"
#include "kernel/vfs.h"
#include "kernel/ata.h"
#include "kernel/pmm.h"
#include "kernel/vmm.h"
#include "kernel/timer.h"
#include "kernel/rtc.h"
#include "kernel/task_scheduler.h"
#include "kernel/thread.h"
#include "kernel/elf_loader.h"
#include "kernel/spinlock.h"
#include "kernel/usermode.h"
#include "kernel/boot_info.h"
#include "kernel/pci.h"
#include "kernel/ahci.h"
#include "kernel/vga.h"
#include "kernel/bga.h"
#include "kernel/pic.h"
#include "kernel/memory_validator.h"
#include "kernel/kmalloc.h"
#include "kernel/fat16.h"
#include "kernel/net.h"
#include "kernel/e1000.h"
#include "kernel/signal.h"
#include "libc.h"

namespace re36 {

static char input_buf[SHELL_MAX_CMD_LEN];
static int input_len = 0;
static int input_cursor = 0;
static int rendered_input_len = 0;
static char current_working_dir[256] = "/";

static volatile uint16_t* vga = (volatile uint16_t*)0xB8000;

static int str_length(const char* str) {
    int len = 0;
    while (str && str[len]) len++;
    return len;
}

static int prompt_len() {
    return str_length(current_working_dir) + 2;
}

static void print_prompt() {
    set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    printf("%s> ", current_working_dir);
    set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    rendered_input_len = 0;
}

static void clear_input_line() {
    term_cursor_set_x(0);
    int visible_len = input_len > rendered_input_len ? input_len : rendered_input_len;
    int len_to_clear = visible_len + prompt_len();
    for (int i = 0; i < len_to_clear; i++) {
        putchar(' ');
    }
    term_cursor_set_x(0);
}

static void redraw_input() {
    clear_input_line();
    printf("\r%s> %s", current_working_dir, input_buf);
    for (int i = input_len; i > input_cursor; i--) {
        putchar('\b');
    }
    rendered_input_len = input_len;
}

static void set_input(const char* str) {
    input_len = 0;
    while (str[input_len] && input_len < SHELL_MAX_CMD_LEN - 1) {
        input_buf[input_len] = str[input_len];
        input_len++;
    }
    input_buf[input_len] = '\0';
    input_cursor = input_len;
    redraw_input();
}

static void reset_input() {
    input_len = 0;
    input_cursor = 0;
    input_buf[0] = '\0';
    rendered_input_len = 0;
}

static void move_cursor_left() {
    if (input_cursor > 0) {
        input_cursor--;
        putchar('\b');
    }
}

static void move_cursor_right() {
    if (input_cursor < input_len) {
        putchar(input_buf[input_cursor]);
        input_cursor++;
    }
}

static void move_cursor_home() {
    while (input_cursor > 0) move_cursor_left();
}

static void move_cursor_end() {
    while (input_cursor < input_len) move_cursor_right();
}

static void insert_input_char(char c) {
    if (input_len >= SHELL_MAX_CMD_LEN - 1) return;

    for (int i = input_len; i > input_cursor; i--) {
        input_buf[i] = input_buf[i - 1];
    }
    input_buf[input_cursor] = c;
    input_len++;
    input_cursor++;
    input_buf[input_len] = '\0';
    redraw_input();
}

static void delete_before_cursor() {
    if (input_cursor <= 0) return;

    for (int i = input_cursor - 1; i < input_len; i++) {
        input_buf[i] = input_buf[i + 1];
    }
    input_len--;
    input_cursor--;
    input_buf[input_len] = '\0';
    redraw_input();
}

static void delete_at_cursor() {
    if (input_cursor >= input_len) return;

    for (int i = input_cursor; i < input_len; i++) {
        input_buf[i] = input_buf[i + 1];
    }
    input_len--;
    input_buf[input_len] = '\0';
    redraw_input();
}

static void clear_current_input() {
    input_len = 0;
    input_cursor = 0;
    input_buf[0] = '\0';
    redraw_input();
}

static bool str_eq(const char* a, const char* b) {
    while (*a && *b) { if (*a++ != *b++) return false; }
    return *a == *b;
}

static bool str_ieq(const char* a, const char* b) {
    while (*a && *b) {
        char ca = *a >= 'a' && *a <= 'z' ? *a - 32 : *a;
        char cb = *b >= 'a' && *b <= 'z' ? *b - 32 : *b;
        if (ca != cb) return false;
        a++; b++;
    }
    char ca = *a >= 'a' && *a <= 'z' ? *a - 32 : *a;
    char cb = *b >= 'a' && *b <= 'z' ? *b - 32 : *b;
    return ca == cb;
}

static bool str_starts(const char* str, const char* prefix, int len) {
    for (int i = 0; i < len; i++) {
        if (str[i] != prefix[i]) return false;
    }
    return true;
}

static const char* path_basename(const char* path) {
    const char* base = path;
    for (int i = 0; path && path[i]; i++) {
        if (path[i] == '/' || path[i] == '\\') base = &path[i + 1];
    }
    return base;
}

static bool is_vesa_app(const char* name) {
    return str_ieq(name, "VESATEST.ELF") || str_ieq(name, "GFXEDIT.ELF");
}

static bool is_driver_app(const char* name) {
    return is_vesa_app(name) ||
           str_ieq(name, "PS2TEST.ELF") ||
           str_ieq(name, "UARTDRV.SYS") ||
           str_ieq(name, "FSDRIVER.ELF");
}

static void grant_mmio_range(Thread& t, uint32_t phys_start, uint32_t phys_end) {
    for (int i = 0; i < t.num_mmio_grants; i++) {
        if (t.allowed_mmio[i].phys_start == phys_start && t.allowed_mmio[i].phys_end == phys_end) return;
    }
    if (t.num_mmio_grants < 8) {
        t.allowed_mmio[t.num_mmio_grants].phys_start = phys_start;
        t.allowed_mmio[t.num_mmio_grants].phys_end = phys_end;
        t.num_mmio_grants++;
    }
}

static void configure_driver_thread(int tid, const char* name) {
    if (tid < 0 || tid >= MAX_THREADS || !name) return;

    Thread& t = threads[tid];
    if (!is_driver_app(name)) return;

    t.is_driver = true;

    if (is_vesa_app(name)) {
        uint32_t lfb = BgaDriver::is_initialized() ? BgaDriver::get_lfb() : 0xA0000;
        grant_mmio_range(t, lfb, lfb + 4 * 1024 * 1024);
    } else if (str_ieq(name, "PS2TEST.ELF")) {
        t.allowed_ports[0].port_start = 0x60;
        t.allowed_ports[0].port_end = 0x64;
        t.num_port_grants = 1;
        t.allowed_irqs[0] = 1;
        t.num_irq_grants = 1;
    } else if (str_ieq(name, "UARTDRV.SYS")) {
        t.allowed_ports[0].port_start = 0x3F8;
        t.allowed_ports[0].port_end = 0x3FF;
        t.num_port_grants = 1;
        t.allowed_irqs[0] = 4;
        t.num_irq_grants = 1;
    }
}

static const char* runall_skip_reason(const char* name) {
    if (str_ieq(name, "GFXEDIT.ELF")) return "interactive graphics editor";
    if (str_ieq(name, "PS2TEST.ELF")) return "interactive PS/2 hardware test";
    if (str_ieq(name, "UARTDRV.SYS")) return "resident interactive UART driver";
    if (str_ieq(name, "STACKBM.ELF")) return "destructive stack guard test";
    return nullptr;
}

static const char* str_after(const char* str, int skip) {
    return str + skip;
}

static bool split_two_args(const char* args, char* first, int first_max, char* second, int second_max) {
    if (!args || !first || !second || first_max <= 0 || second_max <= 0) return false;
    while (*args == ' ') args++;

    int i = 0;
    while (args[i] && args[i] != ' ' && i < first_max - 1) {
        first[i] = args[i];
        i++;
    }
    first[i] = '\0';

    while (args[i] && args[i] != ' ') i++;
    while (args[i] == ' ') i++;

    int j = 0;
    while (args[i] && j < second_max - 1) {
        second[j++] = args[i++];
    }
    while (j > 0 && second[j - 1] == ' ') j--;
    second[j] = '\0';

    return first[0] != '\0' && second[0] != '\0';
}

static const char* read_token(const char* args, char* out, int out_max) {
    if (!out || out_max <= 0) return nullptr;
    if (!args) {
        out[0] = '\0';
        return nullptr;
    }
    while (*args == ' ') args++;
    if (!*args) {
        out[0] = '\0';
        return args;
    }

    int i = 0;
    while (args[i] && args[i] != ' ' && i < out_max - 1) {
        out[i] = args[i];
        i++;
    }
    out[i] = '\0';

    while (args[i] && args[i] != ' ') i++;
    while (args[i] == ' ') i++;
    return args + i;
}

static bool split_three_args(const char* args,
                             char* first, int first_max,
                             char* second, int second_max,
                             char* third, int third_max) {
    const char* rest = read_token(args, first, first_max);
    rest = read_token(rest, second, second_max);
    rest = read_token(rest, third, third_max);
    return first[0] != '\0' && second[0] != '\0' && third[0] != '\0';
}

static void resolve_path(const char* input, char* output) {
    if (!input || !input[0]) {
        int i = 0; while (current_working_dir[i]) { output[i] = current_working_dir[i]; i++; }
        output[i] = '\0';
        return;
    }
    char temp[256];
    int temp_len = 0;
    if (input[0] == '/') {
        int i = 0; while (input[i] && i < 255) { temp[i] = input[i]; i++; }
        temp[i] = '\0';
        temp_len = i;
    } else {
        int i = 0; while (current_working_dir[i] && i < 255) { temp[i] = current_working_dir[i]; i++; }
        if (i > 0 && temp[i - 1] != '/' && i < 255) temp[i++] = '/';
        int j = 0; while (input[j] && i < 255) { temp[i++] = input[j++]; }
        temp[i] = '\0';
        temp_len = i;
    }

    char parts[32][32];
    int num_parts = 0;
    int curr = 0;
    while (curr < temp_len) {
        while (curr < temp_len && temp[curr] == '/') curr++;
        if (curr >= temp_len) break;
        int p = 0;
        while (curr < temp_len && temp[curr] != '/' && p < 31) {
            parts[num_parts][p++] = temp[curr++];
        }
        parts[num_parts][p] = '\0';
        if (str_eq(parts[num_parts], ".")) {
            // Nothing
        } else if (str_eq(parts[num_parts], "..")) {
            if (num_parts > 0) num_parts--;
        } else {
            num_parts++;
        }
    }

    if (num_parts == 0) {
        output[0] = '/';
        output[1] = '\0';
        return;
    }

    int out_len = 0;
    for (int i = 0; i < num_parts; i++) {
        output[out_len++] = '/';
        int p = 0;
        while (parts[i][p]) output[out_len++] = parts[i][p++];
    }
    output[out_len] = '\0';
}

static void print_buffer(const char* data, int len) {
    for (int i = 0; i < len; i++) {
        putchar(data[i]);
    }
}

static int write_shell_output(const char* path, const char* data, int len, bool append, char* resolved_out) {
    char resolved[256];
    resolve_path(path, resolved);
    if (resolved_out) {
        int i = 0;
        while (resolved[i] && i < 255) {
            resolved_out[i] = resolved[i];
            i++;
        }
        resolved_out[i] = '\0';
    }

    if (!append) {
        return vfs_write_file(resolved, (const uint8_t*)data, len);
    }

    int old_len = 0;
    vfs_stat_t st;
    if (vfs_stat(resolved, &st) == 0 && st.type == re36::VnodeType::File) {
        old_len = st.size;
    }

    char* out = (char*)kmalloc(old_len + len + 1);
    if (!out) return -1;

    int actual_old_len = 0;
    if (old_len > 0) {
        vnode* vn = nullptr;
        if (vfs_resolve_path(resolved, &vn) == 0 && vn) {
            while (actual_old_len < old_len && vn->ops && vn->ops->read) {
                int want = old_len - actual_old_len;
                if (want > 512) want = 512;
                int got = vn->ops->read(vn, actual_old_len, (uint8_t*)&out[actual_old_len], want);
                if (got <= 0) break;
                actual_old_len += got;
            }
            vnode_release(vn);
        }
    }

    for (int i = 0; i < len; i++) {
        out[actual_old_len + i] = data[i];
    }
    int result = vfs_write_file(resolved, (const uint8_t*)out, actual_old_len + len);
    kfree(out);
    return result;
}

static bool line_contains(const char* line, int line_len, const char* pattern, int pattern_len) {
    if (pattern_len <= 0) return true;
    if (pattern_len > line_len) return false;

    for (int i = 0; i <= line_len - pattern_len; i++) {
        bool match = true;
        for (int j = 0; j < pattern_len; j++) {
            if (line[i + j] != pattern[j]) {
                match = false;
                break;
            }
        }
        if (match) return true;
    }
    return false;
}

static void pipe_grep(const char* data, int len, const char* pattern) {
    int pattern_len = str_length(pattern);
    int line_start = 0;

    for (int i = 0; i <= len; i++) {
        if (i == len || data[i] == '\n') {
            int line_len = i - line_start;
            if (line_contains(&data[line_start], line_len, pattern, pattern_len)) {
                print_buffer(&data[line_start], line_len);
                if (i < len && data[i] == '\n') putchar('\n');
            }
            line_start = i + 1;
        }
    }
}

static void pipe_head(const char* data, int len, int max_lines) {
    if (max_lines <= 0) max_lines = 10;
    int lines = 0;

    for (int i = 0; i < len; i++) {
        putchar(data[i]);
        if (data[i] == '\n') {
            lines++;
            if (lines >= max_lines) break;
        }
    }
}

static void pipe_count(const char* data, int len) {
    int lines = 0;
    for (int i = 0; i < len; i++) {
        if (data[i] == '\n') lines++;
    }
    printf("lines=%d bytes=%d\n", lines, len);
}

static void exec_command(const char* cmd);

static void run_pipe_sink(const char* cmd, const char* data, int len) {
    if (str_starts(cmd, "write ", 6)) {
        const char* fname = str_after(cmd, 6);
        while (*fname == ' ') fname++;
        if (!*fname) {
            printf("Usage: <cmd> | write <file>\n");
            return;
        }
        char resolved[256];
        if (write_shell_output(fname, data, len, false, resolved) >= 0) {
            printf("Piped %d bytes to %s\n", len, resolved);
        } else {
            printf("Pipe write failed: %s\n", fname);
        }
    } else if (str_starts(cmd, "append ", 7)) {
        const char* fname = str_after(cmd, 7);
        while (*fname == ' ') fname++;
        if (!*fname) {
            printf("Usage: <cmd> | append <file>\n");
            return;
        }
        char resolved[256];
        if (write_shell_output(fname, data, len, true, resolved) >= 0) {
            printf("Appended %d bytes to %s\n", len, resolved);
        } else {
            printf("Pipe append failed: %s\n", fname);
        }
    } else if (str_eq(cmd, "cat") || str_eq(cmd, "cat -")) {
        print_buffer(data, len);
    } else if (str_starts(cmd, "grep ", 5)) {
        const char* pattern = str_after(cmd, 5);
        while (*pattern == ' ') pattern++;
        pipe_grep(data, len, pattern);
    } else if (str_eq(cmd, "head")) {
        pipe_head(data, len, 10);
    } else if (str_starts(cmd, "head ", 5)) {
        int max_lines = atoi(str_after(cmd, 5));
        pipe_head(data, len, max_lines);
    } else if (str_eq(cmd, "count") || str_eq(cmd, "wc")) {
        pipe_count(data, len);
    } else {
        print_buffer(data, len);
        exec_command(cmd);
    }
}

static void run_interactive_write(const char* fname, bool append) {
    int max_len = 16380;
    char* ebuf = (char*)kmalloc(max_len + 4);
    if (!ebuf) {
        printf("Out of memory for editor\n");
        return;
    }
    int elen = 0;
    int orig_len = 0;
    
    if (append) {
        vnode* vn = nullptr;
        if (vfs_resolve_path(fname, &vn) == 0 && vn && vn->ops && vn->ops->read) {
            while (elen < max_len) {
                int r = vn->ops->read(vn, elen, (uint8_t*)&ebuf[elen], max_len - elen > 512 ? 512 : max_len - elen);
                if (r <= 0) break;
                elen += r;
            }
        }
        if (vn) vnode_release(vn);
        orig_len = elen;
        printf("[Appending to %s, %d existing bytes]\n", fname, orig_len);
    } else {
        printf("[Inserting to %s (Interactive)]\n", fname);
    }
    
    printf("TAB=Indent, ENTER=New Line (auto-indents), ESC=Save&Exit\n\n");
    
    int current_indent = 0;
    
    while (true) {
        char c = getchar();
        
        if (c == 27) { // ESC
            break;
        } else if (c == '\n') {
            printf("\n");
            if (elen < max_len) ebuf[elen++] = '\n';
            
            current_indent = 0;
            int i = elen - 2; 
            while (i >= orig_len && ebuf[i] != '\n') i--; 
            i++; 
            while (i < elen - 1 && ebuf[i] == ' ') {
                current_indent++;
                i++;
            }
            
            for (int k = 0; k < current_indent && elen < max_len; k++) {
                printf(" ");
                ebuf[elen++] = ' ';
            }
        } else if (c == '\b') {
            if (elen > orig_len) {
                if (ebuf[elen - 1] != '\n') { 
                    elen--;
                    printf("\b \b");
                }
            }
        } else if (c == '\t') {
            for (int i = 0; i < 4 && elen < max_len; i++) {
                ebuf[elen++] = ' ';
                printf(" ");
            }
        } else if (c >= 32 && c <= 126) {
            if (elen < max_len) {
                ebuf[elen++] = c;
                printf("%c", c);
            }
        }
    }
    
    printf("\nSaving %d bytes...\n", elen);
    if (vfs_write_file(fname, (const uint8_t*)ebuf, elen) >= 0) {
        printf("Written successfully.\n");
    } else {
        printf("Write failed!\n");
    }
    
    kfree(ebuf);
}

static void exec_command(const char* cmd) {
    if (!cmd || !cmd[0]) return;

    if (str_eq(cmd, "hello")) {
        printf("Hello to you too, Kernel Hacker!\n");
    } else if (str_eq(cmd, "clear")) {
        if (VGA::is_graphics()) {
            VGA::clear(0);
        } else {
            for (int i = 0; i < 80 * 25; i++)
                vga[i] = (uint16_t(' ') | (0x0F << 8));
        }
        printf("\n");
    } else if (str_starts(cmd, "cd ", 3)) {
        char resolved[256];
        resolve_path(str_after(cmd, 3), resolved);
        vfs_stat_t st;
        if (vfs_stat(resolved, &st) == 0 && st.type == re36::VnodeType::Directory) {
            int i = 0; while (resolved[i]) { current_working_dir[i] = resolved[i]; i++; }
            current_working_dir[i] = '\0';
        } else {
            printf("cd: no such directory: %s\n", resolved);
        }
    } else if (str_eq(cmd, "up") || str_eq(cmd, "dw")) {
        term_scroll(term_get_max_y() / 2);
    } else if (str_eq(cmd, "upa") || str_eq(cmd, "dwa")) {
        term_scroll(term_get_max_y());
    } else if (str_eq(cmd, "ticks")) {
        printf("Timer ticks: %u\n", Timer::get_ticks());
    } else if (str_eq(cmd, "date")) {
        DateTime dt;
        RTC::read(dt);
        printf("%d-%d-%d %d:%d:%d\n", dt.year, dt.month, dt.day, dt.hours, dt.minutes, dt.seconds);
    } else if (str_eq(cmd, "pci")) {
        PCI::scan_bus();
    } else if (str_starts(cmd, "echo ", 5)) {
        printf("%s\n", str_after(cmd, 5));
    } else if (str_eq(cmd, "uptime")) {
        uint32_t ticks = Timer::get_ticks();
        uint32_t seconds = ticks / 1000; // Timer runs at 1000Hz
        uint32_t minutes = seconds / 60;
        uint32_t hours = minutes / 60;
        printf("Uptime: %u:%u:%u (%u ticks)\n", hours, minutes % 60, seconds % 60, ticks);
    } else if (str_eq(cmd, "reboot")) {
        printf("Rebooting system...\n");
        // Using 8042 keyboard controller to pulse the reset line
        uint8_t good = 0x02;
        while (good & 0x02) {
            good = inb(0x64);
        }
        outb(0x64, 0xFE);
        while(1) { asm volatile ("hlt"); } // Wait for reset
    } else if (str_starts(cmd, "kill ", 5)) {
        int tid = atoi(str_after(cmd, 5));
        printf("Killing thread ID %d...\n", tid);
        re36::thread_terminate(tid);
    } else if (str_eq(cmd, "killall")) {
        int current = TaskScheduler::get_current_tid();
        printf("Terminating all user/background threads...\n");
        for (int i = 2; i < MAX_THREADS; i++) { // Skip 0 (Idle) and 1 (usually kernel init)
            if (i != current && threads[i].state != ThreadState::Unused && threads[i].state != ThreadState::Terminated) {
                printf(" - Killing TID %d\n", i);
                re36::thread_terminate(i);
            }
        }
        printf("Done.\n");
    } else if (str_starts(cmd, "sleep ", 6)) {
        int ms = atoi(str_after(cmd, 6));
        printf("Sleeping for %d ms...\n", ms);
        TaskScheduler::sleep_current(ms);
        printf("Awake!\n");
    } else if (str_eq(cmd, "yield")) {
        re36::thread_yield();
    } else if (str_eq(cmd, "kernelpanic")) {
        printf("KERNEL PANIC: User Requested Panic\n");
        while(1) asm volatile ("cli; hlt");
    } else if (str_eq(cmd, "memtest")) {
        MemoryValidator::run_all_tests();
    } else if (str_eq(cmd, "pmmtest")) {
        MemoryValidator::test_pmm();
    } else if (str_eq(cmd, "vmmtest")) {
        MemoryValidator::test_vmm();
    } else if (str_eq(cmd, "syscall")) {
        printf("Testing int 0x80 (SYS_GETPID)...\n");
        uint32_t tid;
        asm volatile("mov $5, %%eax; int $0x80; mov %%eax, %0" : "=r"(tid) :: "eax");
        printf("Syscall returned TID = %d\n", tid);
    } else if (str_starts(cmd, "ahcitest ", 9)) {
        int port = atoi(str_after(cmd, 9));
        printf("Testing AHCI Port %d...\n", port);
        
        void* test_buf = PhysicalMemoryManager::alloc_frame();
        if (!test_buf) {
            printf("Failed to allocate test buffer!\n");
        } else {
            uint8_t* ptr = (uint8_t*)test_buf;
            for (int i = 0; i < 512; i++) ptr[i] = (i % 256);
            
            printf("Writing sector 10000...\n");
            bool wr_ok = AHCIDriver::write(port, 10000, 1, test_buf);
            if (!wr_ok) printf("Write FAILED!\n");
            else {
                printf("Write OK. Clearing buffer and reading back...\n");
                for (int i = 0; i < 512; i++) ptr[i] = 0;
                
                bool rd_ok = AHCIDriver::read(port, 10000, 1, test_buf);
                if (!rd_ok) printf("Read FAILED!\n");
                else {
                    bool data_ok = true;
                    for (int i = 0; i < 512; i++) {
                        if (ptr[i] != (i % 256)) {
                            data_ok = false;
                            break;
                        }
                    }
                    if (data_ok) printf("Read OK and Data matches perfectly! AHCI works.\n");
                    else printf("Read OK but Data CORRUPTED!\n");
                }
            }
            PhysicalMemoryManager::free_frame(test_buf);
        }
    } else if (str_eq(cmd, "netinfo") || str_eq(cmd, "ifconfig")) {
        E1000Driver::poll();
        NetStack::print_info();
    } else if (str_starts(cmd, "ifconfig ", 9)) {
        char ip_arg[32];
        char mask_arg[32];
        char gw_arg[32];
        if (!split_three_args(str_after(cmd, 9), ip_arg, sizeof(ip_arg),
                              mask_arg, sizeof(mask_arg), gw_arg, sizeof(gw_arg))) {
            printf("Usage: ifconfig <ip> <netmask> <gateway>\n");
        } else {
            bool ok_ip = false;
            bool ok_mask = false;
            bool ok_gw = false;
            uint32_t ip = NetStack::parse_ipv4(ip_arg, &ok_ip);
            uint32_t mask = NetStack::parse_ipv4(mask_arg, &ok_mask);
            uint32_t gw = NetStack::parse_ipv4(gw_arg, &ok_gw);
            if (!ok_ip || !ok_mask || !ok_gw) {
                printf("ifconfig: invalid IPv4 address\n");
            } else {
                NetStack::configure_ipv4(ip, mask, gw);
                NetStack::print_info();
            }
        }
    } else if (str_eq(cmd, "arp")) {
        E1000Driver::poll();
        NetStack::print_arp_cache();
    } else if (str_starts(cmd, "ping ", 5)) {
        bool ok = false;
        uint32_t ip = NetStack::parse_ipv4(str_after(cmd, 5), &ok);
        if (!ok) {
            printf("Usage: ping <ipv4>\n");
        } else if (!NetStack::is_link_up()) {
            printf("ping: network link is down\n");
        } else {
            static uint16_t seq = 1;
            uint16_t id = 0x3636;
            uint16_t this_seq = seq++;
            uint32_t start = Timer::get_ticks();

            if (!NetStack::send_icmp_echo(ip, id, this_seq)) {
                printf("ping: resolving ARP...\n");
                for (int i = 0; i < 10; i++) {
                    Timer::sleep(50);
                    E1000Driver::poll();
                    if (NetStack::send_icmp_echo(ip, id, this_seq)) break;
                }
            }

            bool replied = false;
            uint32_t reply_ip = 0;
            uint8_t ttl = 0;
            for (int i = 0; i < 50; i++) {
                E1000Driver::poll();
                if (NetStack::consume_ping_reply(id, this_seq, &reply_ip, &ttl)) {
                    replied = true;
                    break;
                }
                Timer::sleep(20);
            }

            char ipbuf[16];
            NetStack::format_ipv4(ip, ipbuf);
            if (replied) {
                uint32_t elapsed = Timer::get_ticks() - start;
                char srcbuf[16];
                NetStack::format_ipv4(reply_ip, srcbuf);
                printf("reply from %s: seq=%u ttl=%u time=%u ticks\n", srcbuf, this_seq, ttl, elapsed);
            } else {
                printf("request timeout for %s\n", ipbuf);
            }
        }
    } else if (str_starts(cmd, "udpsend ", 8)) {
        char ip_arg[32];
        char port_arg[16];
        const char* rest = read_token(str_after(cmd, 8), ip_arg, sizeof(ip_arg));
        rest = read_token(rest, port_arg, sizeof(port_arg));
        while (rest && *rest == ' ') rest++;
        if (ip_arg[0] == '\0' || port_arg[0] == '\0' || !rest || rest[0] == '\0') {
            printf("Usage: udpsend <ip> <port> <text>\n");
        } else {
            bool ok = false;
            uint32_t ip = NetStack::parse_ipv4(ip_arg, &ok);
            int port = atoi(port_arg);
            int len = str_length(rest);
            if (!ok || port <= 0 || port > 65535 || len > NET_MAX_UDP_PAYLOAD) {
                printf("udpsend: invalid argument\n");
            } else if (NetStack::send_udp(ip, (uint16_t)port, 49152, (const uint8_t*)rest, (uint16_t)len)) {
                printf("sent %d UDP bytes\n", len);
            } else {
                printf("udpsend: send failed (ARP may still be resolving)\n");
            }
        }
    } else if (str_starts(cmd, "udprecv ", 8)) {
        int port = atoi(str_after(cmd, 8));
        if (port <= 0 || port > 65535) {
            printf("Usage: udprecv <port>\n");
        } else {
            E1000Driver::poll();
            uint8_t buf[NET_MAX_UDP_PAYLOAD + 1];
            uint32_t src_ip = 0;
            uint16_t src_port = 0;
            int got = NetStack::recv_udp((uint16_t)port, &src_ip, &src_port, buf, NET_MAX_UDP_PAYLOAD);
            if (got <= 0) {
                printf("no UDP packets queued for port %d\n", port);
            } else {
                buf[got] = '\0';
                char src[16];
                NetStack::format_ipv4(src_ip, src);
                printf("udp from %s:%u (%d bytes): ", src, src_port, got);
                for (int i = 0; i < got; i++) putchar((buf[i] >= 32 && buf[i] <= 126) ? buf[i] : '.');
                printf("\n");
            }
        }
    } else if (str_eq(cmd, "help")) {
        printf("File: ls <path>, mkdir <path>, cat, less, more, write, rm, mv, stat, hexdump, exec, mknod, link, symlink, readlink, cd <path>\n");
        printf("System: ps (threads), kill, killall, ticks, uptime, date, whoiam, fork\n");
        printf("        meminfo (mems), pci, bootinfo, syscall, ring3, clear, runall <dir>\n");
        printf("Network: netinfo, ifconfig [ip mask gw], arp, ping <ip>, udpsend, udprecv\n");
        printf("        reboot, kernelpanic, echo, sleep, yield, help, helpme\n");
        printf("Tests:  memtest, pmmtest, vmmtest, ahcitest <port>\n");
        printf("Display: mode text, mode gfx, gfx, bga\n");
        printf("Shell: Tab=complete, Up/Down=history, Left/Right/Delete=edit\n");
        printf("       > redirects, >> appends, | cat/grep/head/count/write/append\n");
    } else if (str_eq(cmd, "helpme")) {
        printf("--- VLSMC Shell Extensive Help ---\n");
        printf("[File System Commands]\n");
        printf("  ls [path]         - List directory contents (defaults to cwd if path missing)\n");
        printf("  cd <path>         - Change current working directory\n");
        printf("  mkdir <path>      - Create a new directory\n");
        printf("  rm <path>         - Remove a file or empty directory\n");
        printf("  mv <src> <dst>    - Rename or move a file or directory\n");
        printf("  cat <path>        - Dump entire file content to screen\n");
        printf("  less/more <path>  - Print file content page by page\n");
        printf("  write <name> <v>  - Write literal value <v> into <name> text file\n");
        printf("  writeo <name>     - Open interactive editor to append to file <name>\n");
        printf("  writei <name>     - Open interactive editor to overwrite file <name>\n");
        printf("  stat <path>       - Display file size, clusters, attributes and date\n");
        printf("  hexdump <path>    - Display a 256-byte hex + ascii dump of a file\n");
        printf("  mknod <path>      - Create an empty file\n");
        printf("  link <old> <new>  - Create a hardlink to an existing file\n");
        printf("  symlink <t> <ln>  - Create symbolic link <ln> pointing at <t>\n");
        printf("  readlink <path>   - Print the stored target of a symbolic link\n");
        printf("  chattr [+-]attr   - Change file attributes (+gd, -gd, +gc, -gc)\n");
        printf("\n[Execution & Process Commands]\n");
        printf("  runall <dir>      - Run all .ELF files in a directory sequentially\n");
        printf("  exec <path>       - Execute an ELF binary in user-space\n");
        printf("  ps / threads      - Display running tasks and threads\n");
        printf("  kill <tid>        - Terminate a specific thread by TID\n");
        printf("  killall           - Terminate all user/background threads\n");
        printf("  sleep <ms>        - Put current thread to sleep for <ms> milliseconds\n");
        printf("  yield             - Yield CPU time exactly once\n");
        printf("  fork              - Test fork() mechanism (spawns dummy thread)\n");
        printf("  ring3             - Manually drop to Ring 3 testing mode\n");
        printf("\n[System & Diagnostic]\n");
        printf("  uptime            - Show system uptime in HH:MM:SS (Timer Ticks)\n");
        printf("  ticks             - Print raw timer ticks accumulated\n");
        printf("  date              - Read date & time from CMOS RTC\n");
        printf("  bootinfo          - Display boot contract, memory map and loader info\n");
        printf("  meminfo / mems    - Display memory usage (PMM stats) and Paging state\n");
        printf("  pci               - Scan and enumerate PCI buses/devices\n");
        printf("  whoiam / whoami   - Print current effective user (root)\n");
        printf("  echo <string>     - Print given message to screen\n");
        printf("  syscall           - Issue int 0x80 to test SYS_GETPID bare syscall\n");
        printf("  reboot            - Issue reboot via 8042 keyboard controller\n");
        printf("  kernelpanic       - Purposely trigger a system Kernel Panic\n");
        printf("\n[Network]\n");
        printf("  netinfo           - Show link, MAC, IPv4 and protocol counters\n");
        printf("  ifconfig          - Show current IPv4 configuration\n");
        printf("  ifconfig ip mask gw - Set IPv4 address, netmask and gateway\n");
        printf("  arp               - Display the ARP cache\n");
        printf("  ping <ip>         - Send one ICMP echo request\n");
        printf("  udpsend ip port text - Send one UDP datagram\n");
        printf("  udprecv <port>    - Poll one queued UDP datagram for a local port\n");
        printf("\n[Terminal & UI Commands]\n");
        printf("  clear             - Clear the screen and reset cursor to 0,0\n");
        printf("  up / dw           - Scroll the terminal up/down by half a screen\n");
        printf("  upa / dwa         - Scroll the terminal up/down by a full screen\n");
        printf("  mode text         - Switch VGA to 80x25 text mode (Standard)\n");
        printf("  mode gfx          - Switch VGA to 40x25 graphical fake text mode\n");
        printf("  bga               - Initialize BGA (Bochs Graphics Adapter) 1024x768\n");
        printf("  gfx               - Test pattern or mode 13h depending on active driver\n");
        printf("  Left/Right/Delete - Edit the current input line in place\n");
        printf("  Ctrl-A/E/U/L      - Home, End, clear line, clear screen\n");
        printf("  cmd > file        - Redirect command output to a file\n");
        printf("  cmd >> file       - Append command output to a file\n");
        printf("  cmd | grep text   - Pipe to grep/head/count/cat/write/append\n");
        printf("\n[Built-in Hardware Tests (Destructive/Risky)]\n");
        printf("  memtest           - Run sweeping tests over RAM\n");
        printf("  pmmtest           - Run basic tests on Physical Memory Manager\n");
        printf("  vmmtest           - Run basic tests on Virtual Memory Manager\n");
        printf("  ahcitest <port>   - (Risky) Write/Read Sector 10000 on given AHCI port\n");
    } else if (str_eq(cmd, "gfx")) {
        if (BgaDriver::is_initialized()) {
            uint16_t w = BgaDriver::get_width();
            uint16_t h = BgaDriver::get_height();
            
            // Draw a colorful gradient pattern natively via BGA
            for (uint16_t y = 0; y < h; y++) {
                for (uint16_t x = 0; x < w; x++) {
                    uint8_t r = (x * 255) / w;
                    uint8_t g = (y * 255) / h;
                    uint8_t b = 128 + ((x+y) % 128);
                    BgaDriver::put_pixel(x, y, (r << 16) | (g << 8) | b);
                }
            }
            
            const char* title = "BGA 1024x768 DEMO - Native Graphics Mode";
            for (int i = 0; title[i] != '\0'; i++) {
                BgaDriver::draw_char(20 + i*8, 20, title[i], 0xFFFFFF, 0x000000);
            }
        } else {
            VGA::demo();
        }
    } else if (str_eq(cmd, "bga")) {
        // Alias to 'mode gfx' which handles BGA if initialized (or you can use BgaDriver::init logic here, but mode gfx is safer)
        if (!BgaDriver::is_initialized()) {
            printf("Initializing BGA (1024x768x32)...\n");
            BgaDriver::init(1024, 768, 32);
        } else {
            printf("BGA is already active.\n");
        }
    } else if (str_eq(cmd, "ps") || str_eq(cmd, "threads")) {
        TaskScheduler::print_threads();
    } else if (str_eq(cmd, "meminfo") || str_eq(cmd, "mems")) {
        printf("Managed RAM: %u KB\n", PhysicalMemoryManager::get_managed_memory_limit() / 1024);
        printf("Direct map limit: %u KB\n", PhysicalMemoryManager::get_direct_map_limit() / 1024);
        printf("Direct usable/free: %u/%u KB\n",
               PhysicalMemoryManager::get_direct_mapped_memory() / 1024,
               PhysicalMemoryManager::get_free_direct_mapped_memory() / 1024);
        printf("High usable/free: %u/%u KB\n",
               PhysicalMemoryManager::get_high_memory() / 1024,
               PhysicalMemoryManager::get_free_high_memory() / 1024);
        printf("Total free/used: %u/%u KB\n",
               PhysicalMemoryManager::get_free_memory() / 1024,
               PhysicalMemoryManager::get_used_memory() / 1024);
        uint32_t cr3_val; asm volatile("mov %%cr3, %0" : "=r"(cr3_val));
        printf("Paging: Enabled (CR3 = 0x%x)\n", cr3_val);
    } else if (str_eq(cmd, "mode text")) {
        VGA::init_text_mode();
        set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        printf("Switched to VGA Text Mode (80x25)\n");
    } else if (str_eq(cmd, "mode gfx")) {
        VGA::init_mode13h();
        set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        printf("Switched to VGA Graphics Text Mode (40x25)\n");
    } else if (str_eq(cmd, "bootinfo")) {
        BootInfo* bi = get_boot_info();
        if (boot_info_is_valid(bi)) {
            uint32_t total_usable_kb = 0;
            printf("Boot drive: 0x%x\n", bi->boot_drive);
            printf("Video mode: 0x%x\n", bi->video_mode);
            printf("Boot contract: v%u, size=%u, flags=0x%x\n", bi->version, bi->size, bi->flags);
            printf("Conventional memory: %u KB\n", bi->conventional_memory_kb);
            printf("Kernel load: addr=0x%x size=%u KB entry=0x%x\n",
                   bi->kernel_load_addr,
                   bi->kernel_load_size / 1024,
                   bi->kernel_entry_addr);
            printf("Boot stack: top=0x%x size=%u KB\n", bi->boot_stack_top, bi->boot_stack_size / 1024);
            printf("Memory map entries: %u\n", bi->memory_map_entry_count);
            for (uint16_t i = 0; i < bi->memory_map_entry_count; i++) {
                const BootMemoryMapEntry& entry = bi->memory_map[i];
                if (entry.type == BOOT_MEMORY_TYPE_USABLE && entry.base_high == 0) {
                    total_usable_kb += entry.length_low / 1024;
                }
                printf("  [%u] base=%x:%x len=%x:%x type=%u\n",
                       i,
                       entry.base_high,
                       entry.base_low,
                       entry.length_high,
                       entry.length_low,
                       entry.type);
            }
            printf("Usable RAM below 4 GiB: %u KB (%u MB)\n", total_usable_kb, total_usable_kb / 1024);
            printf("Boot magic: 0x%x (OK)\n", bi->magic);
        } else {
            printf("Boot info not available or invalid (magic: 0x%x)\n", bi ? bi->magic : 0);
        }
    } else if (str_eq(cmd, "ring3")) {
        printf("Launching Ring 3 user process...\n");
        void (*entry)() = []() { enter_usermode(); };
        thread_create("user0", entry, 10);
    } else if (str_starts(cmd, "ls", 2)) {
        char resolved[256];
        if (cmd[2] == ' ' && cmd[3]) resolve_path(str_after(cmd, 3), resolved);
        else resolve_path("", resolved);
        
        vfs_dir_entry dir_entries[VFS_DIR_MAX_ENTRIES];
        int count = vfs_readdir(resolved, dir_entries, VFS_DIR_MAX_ENTRIES);
        if (count < 0) {
            printf("Could not read directory %s. Check if filesystem is mounted and path exists.\n", resolved);
        } else {
            printf("\n  Name          Size     Attr\n");
            printf("  ------------- -------- --------\n");
            for (int pass = 0; pass < 2; pass++) {
                for (int i = 0; i < count; i++) {
                    bool is_dir = (dir_entries[i].type == 'D');
                    if ((pass == 0 && !is_dir) || (pass == 1 && is_dir)) continue;

                    char attrs[16] = "[    ]";
                    if (is_dir) attrs[1] = 'D';
                    if (dir_entries[i].attributes & 0x01) attrs[2] = 'R';
                    if (dir_entries[i].attributes & 0x02) attrs[3] = 'H';
                    if (dir_entries[i].attributes & 0x40) { attrs[4] = '-'; attrs[5] = 'g'; attrs[6] = 'c'; attrs[7] = ' '; }
                    if (dir_entries[i].attributes & 0x80) { attrs[8] = '-'; attrs[9] = 'g'; attrs[10] = 'd'; }
                    attrs[11] = '\0';

                    if (is_dir) {
                        printf("  %-13s    <DIR>\t %s\n", dir_entries[i].name, attrs);
                    } else {
                        printf("  %-13s %8d B\t %s\n", dir_entries[i].name, dir_entries[i].size, attrs);
                    }
                }
            }
            printf("\n  Total: %d entries\n\n", count);
        }
    } else if (str_starts(cmd, "mkdir ", 6)) {
        char resolved[256];
        resolve_path(str_after(cmd, 6), resolved);
        if (vfs_mkdir(resolved, 0) == 0) {
            printf("Directory %s created successfully.\n", resolved);
        } else {
            printf("Failed to create directory %s.\n", resolved);
        }
    } else if (str_starts(cmd, "mv ", 3)) {
        const char* args = str_after(cmd, 3);
        char src[256];
        char dest[256];
        int i = 0, j = 0;
        while (args[i] && args[i] != ' ') {
            src[j++] = args[i++];
        }
        src[j] = '\0';
        while (args[i] == ' ') i++;
        j = 0;
        while (args[i]) {
            dest[j++] = args[i++];
        }
        dest[j] = '\0';
        
        if (src[0] == '\0' || dest[0] == '\0') {
            printf("Usage: mv <src> <dest>\n");
        } else {
            vfs_stat_t st;
            if (vfs_stat(dest, &st) == 0 && st.type == re36::VnodeType::Directory) {
                const char* basename = src;
                for (int m = 0; src[m]; m++) {
                    if (src[m] == '/') basename = &src[m + 1];
                }
                int dlen = 0; while (dest[dlen]) dlen++;
                if (dest[dlen - 1] != '/') dest[dlen++] = '/';
                for (int m = 0; basename[m]; m++) dest[dlen++] = basename[m];
                dest[dlen] = '\0';
            }
            
            if (vfs_rename(src, dest) == 0) {
                printf("Moved %s to %s successfully.\n", src, dest);
            } else {
                printf("Failed to move %s to %s.\n", src, dest);
            }
        }
    } else if (str_starts(cmd, "exec ", 5)) {
        char resolved[256];
        resolve_path(str_after(cmd, 5), resolved);
        const char* app_name = path_basename(resolved);
        
        int tid = -1;
        {
            InterruptGuard guard;
            tid = elf_exec(resolved, is_driver_app(app_name));
            if (tid >= 0) {
                configure_driver_thread(tid, app_name);
            }
        }
        
        if (tid >= 0) {
            Signal::set_foreground_tid(tid);
            TaskScheduler::join(tid);
            Signal::set_foreground_tid(-1);
        }
    } else if (str_starts(cmd, "cat ", 4)) {
        static uint8_t file_buf[4096];
        vnode* vn = nullptr;
        char resolved[256];
        resolve_path(str_after(cmd, 4), resolved);
        if (vfs_resolve_path(resolved, &vn) != 0 || !vn) {
            printf("File not found: %s\n", resolved);
        } else {
            int bytes = -1;
            if (vn->ops && vn->ops->read)
                bytes = vn->ops->read(vn, 0, file_buf, sizeof(file_buf) - 1);
            vnode_release(vn);
            if (bytes < 0) {
                printf("Read failed: %s\n", resolved);
            } else {
                file_buf[bytes] = '\0';
                printf("%s\n", (const char*)file_buf);
            }
        }
    } else if (str_starts(cmd, "less ", 5) || str_starts(cmd, "more ", 5)) {
        char resolved[256];
        resolve_path(str_after(cmd, 5), resolved);
        vnode* vn = nullptr;
        if (vfs_resolve_path(resolved, &vn) != 0 || !vn) {
            printf("File not found: %s\n", resolved);
        } else {
            int max_lines = 23;
            int lines = 0;
            uint32_t offset = 0;
            bool eof = false;
            while (!eof) {
                uint8_t buf[64];
                int bytes = -1;
                if (vn->ops && vn->ops->read) {
                    bytes = vn->ops->read(vn, offset, buf, sizeof(buf));
                }
                if (bytes <= 0) break;
                
                for (int i = 0; i < bytes; i++) {
                    putchar(buf[i]);
                    if (buf[i] == '\n') {
                        lines++;
                        if (lines >= max_lines) {
                            set_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY);
                            printf("--More--");
                            set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
                            char c = getchar();
                            printf("\r        \r");
                            if (c == 'q' || c == 'Q') {
                                eof = true;
                                break;
                            }
                            lines = 0;
                        }
                    }
                }
                offset += bytes;
            }
            if (!eof && lines > 0) printf("\n");
            vnode_release(vn);
        }
    } else if (str_starts(cmd, "chattr ", 7)) {
        const char* args = str_after(cmd, 7);
        while (*args == ' ') args++;
        
        bool set = false;
        bool rem = false;
        uint8_t flag = 0;
        
        if (str_starts(args, "+gd ", 4)) { set = true; flag = 0x80; args += 4; }
        else if (str_starts(args, "-gd ", 4)) { rem = true; flag = 0x80; args += 4; }
        else if (str_starts(args, "+gc ", 4)) { set = true; flag = 0x40; args += 4; }
        else if (str_starts(args, "-gc ", 4)) { rem = true; flag = 0x40; args += 4; }
        
        while (*args == ' ') args++;
        if (!*args || (!set && !rem)) {
            printf("Usage: chattr <+gd|-gd|+gc|-gc> <filename>\n");
        } else {
            char resolved[256];
            resolve_path(args, resolved);
            vnode* vn = nullptr;
            if (vfs_resolve_path(resolved, &vn) == 0 && vn) {
                if (re36::Fat16::change_attributes(vn, flag, set)) {
                    printf("Attributes updated.\n");
                } else {
                    printf("Failed to update attributes.\n");
                }
                vnode_release(vn);
            } else {
                printf("File not found: %s\n", resolved);
            }
        }
    } else if (str_eq(cmd, "whoiam") || str_eq(cmd, "whoami")) {
        printf("root\n");
    } else if (str_starts(cmd, "mknod ", 6)) {
        char resolved[256];
        resolve_path(str_after(cmd, 6), resolved);
        if (vfs_write_file(resolved, (const uint8_t*)"", 0) >= 0) {
            printf("Created empty file: %s\n", resolved);
        } else {
            printf("Failed to create file: %s\n", resolved);
        }
    } else if (str_starts(cmd, "link ", 5)) {
        char old_arg[128];
        char new_arg[128];
        if (!split_two_args(str_after(cmd, 5), old_arg, sizeof(old_arg), new_arg, sizeof(new_arg))) {
            printf("Usage: link <oldpath> <newpath>\n");
        } else {
            char old_resolved[256];
            char new_resolved[256];
            resolve_path(old_arg, old_resolved);
            resolve_path(new_arg, new_resolved);
            if (vfs_link(old_resolved, new_resolved) == 0) {
                printf("Linked %s -> %s\n", new_resolved, old_resolved);
            } else {
                printf("link failed: %s -> %s\n", new_resolved, old_resolved);
            }
        }
    } else if (str_starts(cmd, "symlink ", 8)) {
        char target[256];
        char link_arg[128];
        if (!split_two_args(str_after(cmd, 8), target, sizeof(target), link_arg, sizeof(link_arg))) {
            printf("Usage: symlink <target> <linkpath>\n");
        } else {
            char link_resolved[256];
            resolve_path(link_arg, link_resolved);
            if (vfs_symlink(target, link_resolved) == 0) {
                printf("Symlink %s -> %s\n", link_resolved, target);
            } else {
                printf("symlink failed: %s -> %s\n", link_resolved, target);
            }
        }
    } else if (str_starts(cmd, "readlink ", 9)) {
        char resolved[256];
        resolve_path(str_after(cmd, 9), resolved);
        char target[256];
        int len = vfs_readlink(resolved, target, sizeof(target));
        if (len >= 0) {
            printf("%s\n", target);
        } else {
            printf("readlink failed: %s\n", resolved);
        }
    } else if (str_eq(cmd, "fork")) {
        printf("Forking test thread...\n");
        auto fork_test_entry = []() {
            printf("\n[fork] Child thread running! PID = %d\n", TaskScheduler::get_current_tid());
        };
        int child = thread_create("fork_test", fork_test_entry, 10);
        printf("Spawned child with TID %d\n", child);
    } else if (str_starts(cmd, "write ", 6)) {
        const char* args = str_after(cmd, 6);
        const char* space = args;
        while (*space && *space != ' ') space++;
        if (*space == ' ') {
            int name_len = space - args;
            char fname[32];
            for (int k = 0; k < name_len && k < 31; k++) fname[k] = args[k];
            fname[name_len < 31 ? name_len : 31] = '\0';
            char resolved[256];
            resolve_path(fname, resolved);
            const char* content = space + 1;
            int clen = 0;
            while (content[clen]) clen++;
            if (vfs_write_file(resolved, (const uint8_t*)content, clen) >= 0)
                printf("Written %d bytes to %s\n", clen, resolved);
            else
                printf("Write failed!\n");
        } else {
            printf("Usage: write <filename> <content>\n");
        }
    } else if (str_starts(cmd, "writeo ", 7)) {
        const char* args = str_after(cmd, 7);
        while (*args == ' ') args++;
        char wfname[32];
        int wi = 0;
        while (args[wi] && args[wi] != ' ' && wi < 31) { wfname[wi] = args[wi]; wi++; }
        wfname[wi] = '\0';
        char resolved[256];
        resolve_path(wfname, resolved);
        if (wfname[0]) run_interactive_write(resolved, true);
        else printf("Usage: writeo <filename>\n");
    } else if (str_starts(cmd, "writei ", 7)) {
        const char* args = str_after(cmd, 7);
        while (*args == ' ') args++;
        char wfname[32];
        int wi = 0;
        while (args[wi] && args[wi] != ' ' && wi < 31) { wfname[wi] = args[wi]; wi++; }
        wfname[wi] = '\0';
        char resolved[256];
        resolve_path(wfname, resolved);
        if (wfname[0]) run_interactive_write(resolved, false);
        else printf("Usage: writei <filename>\n");
    } else if (str_starts(cmd, "rm ", 3)) {
        char resolved[256];
        resolve_path(str_after(cmd, 3), resolved);
        if (vfs_unlink(resolved) == 0)
            printf("Deleted: %s\n", resolved);
    } else if (str_starts(cmd, "stat ", 5)) {
        char resolved[256];
        resolve_path(str_after(cmd, 5), resolved);
        vfs_stat_t st;
        if (vfs_stat(resolved, &st) != 0) {
            printf("File not found: %s\n", resolved);
        } else {
            printf("\n  File: %s\n", resolved);
            printf("  Size: %d bytes\n", st.size);
            printf("  Links: %d\n", st.nlink);
            printf("  Cluster: %d\n", st.first_cluster);
            printf("  Attr: ");
            if (st.attributes & 0x01) printf("R ");
            if (st.attributes & 0x02) printf("H ");
            if (st.attributes & 0x04) printf("S ");
            if (st.attributes & 0x10) printf("D ");
            if (st.attributes & 0x20) printf("A ");
            if (st.attributes & 0x40) printf("-gc(ModifyProtect) ");
            if (st.attributes & 0x80) printf("-gd(DeleteProtect) ");
            printf("\n");
            uint16_t t = st.mod_time;
            uint16_t d = st.mod_date;
            printf("  Modified: %d-%d-%d %d:%d:%d\n",
                1980 + (d >> 9), (d >> 5) & 0xF, d & 0x1F,
                t >> 11, (t >> 5) & 0x3F, (t & 0x1F) * 2);
            printf("\n");
        }
    } else if (str_starts(cmd, "hexdump ", 8)) {
        static uint8_t hbuf[256];
        vnode* hvn = nullptr;
        int bytes = -1;
        char resolved[256];
        resolve_path(str_after(cmd, 8), resolved);
        if (vfs_resolve_path(resolved, &hvn) == 0 && hvn) {
            if (hvn->ops && hvn->ops->read)
                bytes = hvn->ops->read(hvn, 0, hbuf, sizeof(hbuf));
            vnode_release(hvn);
        }
        if (bytes < 0) {
            printf("File not found: %s\n", resolved);
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
    } else if (str_starts(cmd, "runall ", 7)) {
        char resolved[256];
        resolve_path(str_after(cmd, 7), resolved);
        vfs_dir_entry dir_entries[VFS_DIR_MAX_ENTRIES];
        int count = vfs_readdir(resolved, dir_entries, VFS_DIR_MAX_ENTRIES);
        if (count < 0) {
            printf("Could not read directory %s.\n", resolved);
        } else {
            int fsdriver_tid = -1;
            for (int i = 0; i < count; i++) {
                if (dir_entries[i].type != 'D') {
                    char fullpath[256];
                    int len = 0;
                    while (resolved[len] && len < 200) { fullpath[len] = resolved[len]; len++; }
                    if (len > 0 && fullpath[len-1] != '/') fullpath[len++] = '/';
                    int nlen = 0;
                    while (dir_entries[i].name[nlen] && len < 255) { fullpath[len++] = dir_entries[i].name[nlen++]; }
                    fullpath[len] = '\0';
                    
                    if (str_eq(dir_entries[i].name, "LD.SO") || str_eq(dir_entries[i].name, "LIBC.SO") || str_eq(dir_entries[i].name, "LIBTEST.SO")) {
                        continue;
                    }
                    
                    if (len >= 4 && (str_ieq(&fullpath[len-4], ".ELF") || str_ieq(&fullpath[len-4], ".SYS"))) {
                        if (str_ieq(dir_entries[i].name, "FSDRIVER.ELF")) {
                            if (fsdriver_tid < 0) {
                                printf("\n========================================\n");
                                printf("=== Starting %s in background\n", fullpath);
                                printf("========================================\n");
                                InterruptGuard guard;
                                fsdriver_tid = elf_exec(fullpath, true);
                                if (fsdriver_tid >= 0) configure_driver_thread(fsdriver_tid, dir_entries[i].name);
                            }
                            continue;
                        }

                        const char* skip_reason = runall_skip_reason(dir_entries[i].name);
                        if (skip_reason) {
                            printf("\n--- Skipping %s (%s) ---\n", fullpath, skip_reason);
                            continue;
                        }

                        printf("\n========================================\n");
                        printf("=== Running %s \n", fullpath);
                        printf("========================================\n");
                        
                        int tid = -1;
                        {
                            InterruptGuard guard;
                            tid = elf_exec(fullpath, is_driver_app(dir_entries[i].name));
                            if (tid >= 0) {
                                configure_driver_thread(tid, dir_entries[i].name);
                            }
                        }
                        
                        if (tid >= 0) {
                            Signal::set_foreground_tid(tid);
                            TaskScheduler::join(tid);
                            Signal::set_foreground_tid(-1);
                        }
                    }
                }
            }
            if (fsdriver_tid >= 0 &&
                threads[fsdriver_tid].state != ThreadState::Unused &&
                threads[fsdriver_tid].state != ThreadState::Terminated) {
                thread_terminate(fsdriver_tid);
            }
            printf("\n--- runall finished ---\n");
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
    char redir_file[128];
    bool append_redirect = false;

    if (ShellRedirect::parse_pipe(line, cmd1, SHELL_MAX_CMD_LEN, cmd2, SHELL_MAX_CMD_LEN)) {
        ShellRedirect::begin_capture();
        exec_command(cmd1);
        ShellRedirect::end_capture();

        const char* pipe_data = ShellRedirect::get_buffer();
        int pipe_len = ShellRedirect::get_length();
        bool truncated = ShellRedirect::has_overflowed();

        run_pipe_sink(cmd2, pipe_data, pipe_len);
        if (truncated) {
            printf("\n[pipe warning: output truncated to %d bytes]\n", pipe_len);
        }
        return;
    }

    if (ShellRedirect::parse(line, cmd1, SHELL_MAX_CMD_LEN, redir_file, sizeof(redir_file), &append_redirect)) {
        ShellRedirect::begin_capture();
        exec_command(cmd1);
        ShellRedirect::end_capture();

        int len = ShellRedirect::get_length();
        bool truncated = ShellRedirect::has_overflowed();
        char resolved[256];
        if (write_shell_output(redir_file, ShellRedirect::get_buffer(), len, append_redirect, resolved) >= 0) {
            printf("%s %d bytes to %s\n", append_redirect ? "Appended" : "Redirected", len, resolved);
            if (truncated) {
                printf("[redirect warning: output truncated to %d bytes]\n", len);
            }
        } else {
            printf("Redirect failed: %s\n", redir_file);
        }
        return;
    }

    exec_command(line);
}

void shell_main() {
    ShellHistory::init();

    set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    printf("RAND Elecorner 36 Shell v2.0\n");
    set_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK);
    printf("Type 'help' for commands. Tab=complete, Up/Down=history\n\n");
    
    print_prompt();

    reset_input();
    
    while (true) {
        char c = getchar();

        if (c == '\n') {
            printf("\n");
            set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
            process_line(input_buf);
            ShellHistory::reset_cursor();
            reset_input();
            print_prompt();
        } else if (c == '\b') {
            delete_before_cursor();
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
        } else if (c == (char)0x82) {
            move_cursor_left();
        } else if (c == (char)0x83) {
            move_cursor_right();
        } else if (c == (char)0x84) {
            delete_at_cursor();
        } else if (c == 1) {
            move_cursor_home();
        } else if (c == 5) {
            move_cursor_end();
        } else if (c == 21) {
            clear_current_input();
        } else if (c == 12) {
            printf("\n");
            set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
            exec_command("clear");
            print_prompt();
            redraw_input();
        } else if (c >= 32 && c <= 126) {
            insert_input_char(c);
        }
    }
}

} // namespace re36
