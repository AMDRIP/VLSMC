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
#include "libc.h"

namespace re36 {

static char input_buf[SHELL_MAX_CMD_LEN];
static int input_len = 0;
static int cursor_x = 0;
static char current_working_dir[256] = "/";

static volatile uint16_t* vga = (volatile uint16_t*)0xB8000;

static void clear_input_line() {
    term_cursor_set_x(0);
    int cwd_len = 0; while (current_working_dir[cwd_len]) cwd_len++;
    int len_to_clear = input_len + cwd_len + 2;
    for (int i = 0; i < len_to_clear; i++) {
        putchar(' ');
    }
    term_cursor_set_x(0);
}

static void redraw_input() {
    clear_input_line();
    printf("\r%s> %s", current_working_dir, input_buf);
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

static const char* str_after(const char* str, int skip) {
    return str + skip;
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

        if (num_parts >= 32) {
            // Path is deeper than we can represent in the shell resolver.
            // Skip the rest to avoid stack corruption.
            while (curr < temp_len && temp[curr] != '/') curr++;
            continue;
        }

        int p = 0;
        while (curr < temp_len && temp[curr] != '/' && p < 31) {
            parts[num_parts][p++] = temp[curr++];
        }
        // Consume overly long segment tail so parser stays in sync.
        while (curr < temp_len && temp[curr] != '/') curr++;

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
        if (out_len >= 255) break;
        output[out_len++] = '/';
        int p = 0;
        while (parts[i][p] && out_len < 255) output[out_len++] = parts[i][p++];
    }
    output[out_len] = '\0';
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
    } else if (str_eq(cmd, "help")) {
        printf("File: ls <path>, mkdir <path>, cat, less, more, write, rm, mv, stat, hexdump, exec, mknod, link, cd <path>\n");
        printf("System: ps (threads), kill, killall, ticks, uptime, date, whoiam, fork\n");
        printf("        meminfo (mems), pci, bootinfo, syscall, ring3, clear, runall <dir>\n");
        printf("        reboot, kernelpanic, echo, sleep, yield, help, helpme\n");
        printf("Tests:  memtest, pmmtest, vmmtest, ahcitest <port>\n");
        printf("Display: mode text, mode gfx, gfx, bga\n");
        printf("Shell: Tab=autocomplete, Up/Down=history, >=redirect, |=pipe, dw, dwa\n");
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
        printf("  link              - (Not Supported) Create symlink/hardlink\n");
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
        printf("  bootinfo          - Display MultiBoot structures (Mem, Video, Drive)\n");
        printf("  meminfo / mems    - Display memory usage (PMM stats) and Paging state\n");
        printf("  pci               - Scan and enumerate PCI buses/devices\n");
        printf("  whoiam / whoami   - Print current effective user (root)\n");
        printf("  echo <string>     - Print given message to screen\n");
        printf("  syscall           - Issue int 0x80 to test SYS_GETPID bare syscall\n");
        printf("  reboot            - Issue reboot via 8042 keyboard controller\n");
        printf("  kernelpanic       - Purposely trigger a system Kernel Panic\n");
        printf("\n[Terminal & UI Commands]\n");
        printf("  clear             - Clear the screen and reset cursor to 0,0\n");
        printf("  up / dw           - Scroll the terminal up/down by half a screen\n");
        printf("  upa / dwa         - Scroll the terminal up/down by a full screen\n");
        printf("  mode text         - Switch VGA to 80x25 text mode (Standard)\n");
        printf("  mode gfx          - Switch VGA to 40x25 graphical fake text mode\n");
        printf("  bga               - Initialize BGA (Bochs Graphics Adapter) 1024x768\n");
        printf("  gfx               - Test pattern or mode 13h depending on active driver\n");
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
        
        int tid = -1;
        {
            InterruptGuard guard;
            tid = elf_exec(resolved);
            if (tid >= 0) {
                int len = 0; while (resolved[len]) len++;
                if (len >= 11 && str_ieq(&resolved[len-11], "PS2TEST.ELF")) {
                    threads[tid].is_driver = true;
                    threads[tid].allowed_ports[0].port_start = 0x60;
                    threads[tid].allowed_ports[0].port_end = 0x64;
                    threads[tid].num_port_grants = 1;
                    threads[tid].allowed_irqs[0] = 1;
                    threads[tid].num_irq_grants = 1;
                } else if (len >= 11 && str_ieq(&resolved[len-11], "UARTDRV.ELF")) {
                    threads[tid].is_driver = true;
                    threads[tid].allowed_ports[0].port_start = 0x3F8;
                    threads[tid].allowed_ports[0].port_end = 0x3FF;
                    threads[tid].num_port_grants = 1;
                    threads[tid].allowed_irqs[0] = 4;
                    threads[tid].num_irq_grants = 1;
                }
            }
        }
        
        if (tid >= 0) {
            TaskScheduler::join(tid);
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
                    if (str_eq(dir_entries[i].name, "FSDRIVER.ELF") || str_eq(dir_entries[i].name, "ANIM.ELF")) {
                        continue;
                    }
                    
                    if (len >= 4 && str_ieq(&fullpath[len-4], ".ELF")) {
                        printf("\n========================================\n");
                        printf("=== Running %s \n", fullpath);
                        printf("========================================\n");
                        
                        int tid = -1;
                        {
                            InterruptGuard guard;
                            tid = elf_exec(fullpath);
                            if (tid >= 0) {
                                if (str_ieq(dir_entries[i].name, "PS2TEST.ELF")) {
                                    threads[tid].is_driver = true;
                                    threads[tid].allowed_ports[0].port_start = 0x60;
                                    threads[tid].allowed_ports[0].port_end = 0x64;
                                    threads[tid].num_port_grants = 1;
                                    threads[tid].allowed_irqs[0] = 1;
                                    threads[tid].num_irq_grants = 1;
                                } else if (str_ieq(dir_entries[i].name, "UARTDRV.ELF")) {
                                    threads[tid].is_driver = true;
                                    threads[tid].allowed_ports[0].port_start = 0x3F8;
                                    threads[tid].allowed_ports[0].port_end = 0x3FF;
                                    threads[tid].num_port_grants = 1;
                                    threads[tid].allowed_irqs[0] = 4;
                                    threads[tid].num_irq_grants = 1;
                                }
                            }
                        }
                        
                        if (tid >= 0) {
                            TaskScheduler::join(tid);
                        }
                    }
                }
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
                printf("\b \b");
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
                putchar(c);
            }
        }
    }
}

} // namespace re36
