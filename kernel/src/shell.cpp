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
#include "kernel/usermode.h"
#include "kernel/boot_info.h"
#include "kernel/pci.h"
#include "kernel/ahci.h"
#include "kernel/vga.h"
#include "kernel/bga.h"
#include "kernel/pic.h"
#include "kernel/memory_validator.h"
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
        if (VGA::is_graphics()) {
            VGA::clear(0);
        } else {
            for (int i = 0; i < 80 * 25; i++)
                vga[i] = (uint16_t(' ') | (0x0F << 8));
        }
        printf("\n");
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
    } else if (str_eq(cmd, "help")) {
        printf("File: ls, cat, less, more, write, rm, stat, hexdump, exec, mknod, link\n");
        printf("System: ps (threads), kill, killall, ticks, uptime, date, whoiam, fork\n");
        printf("        meminfo (mems), pci, bootinfo, syscall, ring3, clear\n");
        printf("        reboot, kernelpanic, echo, sleep, yield, help\n");
        printf("Tests:  memtest, pmmtest, vmmtest, ahcitest <port>\n");
        printf("Display: mode text, mode gfx, gfx, bga\n");
        printf("Shell: Tab=autocomplete, Up/Down=history, >=redirect, |=pipe\n");
    } else if (str_eq(cmd, "gfx")) {
        VGA::demo();
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
        printf("Free RAM: %u KB\n", PhysicalMemoryManager::get_free_memory() / 1024);
        printf("Used RAM: %u KB\n", PhysicalMemoryManager::get_used_memory() / 1024);
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
        if (bi->magic == BOOT_INFO_MAGIC) {
            printf("Boot drive: 0x%x\n", bi->boot_drive);
            printf("Video mode: 0x%x\n", bi->video_mode);
            uint32_t total_kb = 1024 + bi->mem_below_16m_kb + (uint32_t)bi->mem_above_16m_64kb * 64;
            printf("Memory: %u KB (%u MB)\n", total_kb, total_kb / 1024);
            printf("Boot magic: 0x%x (OK)\n", bi->magic);
        } else {
            printf("Boot info not available (magic: 0x%x)\n", bi->magic);
        }
    } else if (str_eq(cmd, "ring3")) {
        printf("Launching Ring 3 user process...\n");
        void (*entry)() = []() { enter_usermode(); };
        thread_create("user0", entry, 10);
    } else if (str_eq(cmd, "ls")) {
        vfs_dir_entry dir_entries[VFS_DIR_MAX_ENTRIES];
        int count = vfs_readdir("/", dir_entries, VFS_DIR_MAX_ENTRIES);
        if (count < 0) {
            printf("No filesystem mounted\n");
        } else {
            printf("\n  Name          Size     Type\n");
            printf("  ------------- -------- ----\n");
            for (int i = 0; i < count; i++) {
                printf("  [%c] %s\t%d B\n", dir_entries[i].type, dir_entries[i].name, dir_entries[i].size);
            }
            printf("\n  Total: %d entries\n\n", count);
        }
    } else if (str_starts(cmd, "exec ", 5)) {
        elf_exec(str_after(cmd, 5));
    } else if (str_starts(cmd, "cat ", 4)) {
        static uint8_t file_buf[4096];
        vnode* vn = nullptr;
        if (vfs_resolve_path(str_after(cmd, 4), &vn) != 0 || !vn) {
            printf("File not found: %s\n", str_after(cmd, 4));
        } else {
            int bytes = -1;
            if (vn->ops && vn->ops->read)
                bytes = vn->ops->read(vn, 0, file_buf, sizeof(file_buf) - 1);
            vnode_release(vn);
            if (bytes < 0) {
                printf("Read failed: %s\n", str_after(cmd, 4));
            } else {
                file_buf[bytes] = '\0';
                printf("%s\n", (const char*)file_buf);
            }
        }
    } else if (str_starts(cmd, "less ", 5) || str_starts(cmd, "more ", 5)) {
        const char* fname = str_after(cmd, 5);
        vnode* vn = nullptr;
        if (vfs_resolve_path(fname, &vn) != 0 || !vn) {
            printf("File not found: %s\n", fname);
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
    } else if (str_eq(cmd, "whoiam") || str_eq(cmd, "whoami")) {
        printf("root\n");
    } else if (str_starts(cmd, "mknod ", 6)) {
        const char* fname = str_after(cmd, 6);
        if (vfs_write_file(fname, (const uint8_t*)"", 0) >= 0) {
            printf("Created empty file: %s\n", fname);
        } else {
            printf("Failed to create file: %s\n", fname);
        }
    } else if (str_starts(cmd, "link ", 5)) {
        printf("link/symlink is not supported on this filesystem (FAT16)\n");
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
            const char* content = space + 1;
            int clen = 0;
            while (content[clen]) clen++;
            if (vfs_write_file(fname, (const uint8_t*)content, clen) >= 0)
                printf("Written %d bytes to %s\n", clen, fname);
            else
                printf("Write failed!\n");
        } else {
            printf("Usage: write <filename> <content>\n");
        }
    } else if (str_starts(cmd, "rm ", 3)) {
        if (vfs_unlink(str_after(cmd, 3)) == 0)
            printf("Deleted: %s\n", str_after(cmd, 3));
    } else if (str_starts(cmd, "stat ", 5)) {
        vfs_stat_t st;
        if (vfs_stat(str_after(cmd, 5), &st) != 0) {
            printf("File not found: %s\n", str_after(cmd, 5));
        } else {
            printf("\n  File: %s\n", str_after(cmd, 5));
            printf("  Size: %d bytes\n", st.size);
            printf("  Cluster: %d\n", st.first_cluster);
            printf("  Attr: ");
            if (st.attributes & 0x01) printf("R ");
            if (st.attributes & 0x02) printf("H ");
            if (st.attributes & 0x04) printf("S ");
            if (st.attributes & 0x10) printf("D ");
            if (st.attributes & 0x20) printf("A ");
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
        if (vfs_resolve_path(str_after(cmd, 8), &hvn) == 0 && hvn) {
            if (hvn->ops && hvn->ops->read)
                bytes = hvn->ops->read(hvn, 0, hbuf, sizeof(hbuf));
            vnode_release(hvn);
        }
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
            if (vfs_write_file(fname, (const uint8_t*)pipe_data, plen) >= 0)
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
        if (vfs_write_file(redir_file, (const uint8_t*)ShellRedirect::get_buffer(), len) >= 0)
            printf("Redirected %d bytes to %s\n", len, redir_file);
        else
            printf("Redirect failed!\n");
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
    
    set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    printf("> ");
    set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    input_len = 0;
    input_buf[0] = '\0';

    while (true) {
        char c = getchar();

        if (c == '\n') {
            printf("\n");
            set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
            process_line(input_buf);
            ShellHistory::reset_cursor();
            input_len = 0;
            input_buf[0] = '\0';
            set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
            printf("> ");
            set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
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
