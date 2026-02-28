#include "kernel/shell_autocomplete.h"
#include "kernel/shell_history.h"
#include "kernel/vfs.h"
#include "libc.h"

namespace re36 {

static const char* builtin_cmds[] = {
    "hello", "clear", "ps", "ticks", "meminfo", "date",
    "syscall", "help", "gfx", "mode text", "mode gfx", "bootinfo",
    "ring3", "ls", "exec", "cat", "write", "rm", "stat", "hexdump", "pci", nullptr
};

static bool starts_with(const char* str, const char* prefix) {
    while (*prefix) {
        if (*str != *prefix) return false;
        str++;
        prefix++;
    }
    return true;
}

const char* ShellAutocomplete::complete(const char* partial) {
    if (!partial || !partial[0]) return nullptr;

    int plen = 0;
    while (partial[plen]) plen++;

    bool has_space = false;
    for (int i = 0; i < plen; i++) {
        if (partial[i] == ' ') { has_space = true; break; }
    }

    if (!has_space) {
        for (int i = 0; builtin_cmds[i]; i++) {
            if (starts_with(builtin_cmds[i], partial)) {
                return builtin_cmds[i];
            }
        }
        return nullptr;
    }

    const char* file_part = partial;
    int last_space = 0;
    for (int i = 0; i < plen; i++) {
        if (partial[i] == ' ') last_space = i;
    }
    file_part = partial + last_space + 1;
    int flen = 0;
    while (file_part[flen]) flen++;

    if (flen == 0) return nullptr;

    static char result[SHELL_MAX_CMD_LEN];
    vfs_dir_entry dir_entries[VFS_DIR_MAX_ENTRIES];
    int count = vfs_readdir("/", dir_entries, VFS_DIR_MAX_ENTRIES);

    for (int i = 0; i < count; i++) {
        bool match = true;
        for (int k = 0; k < flen; k++) {
            char a = file_part[k];
            char b = dir_entries[i].name[k];
            if (a >= 'a' && a <= 'z') a -= 32;
            if (b >= 'a' && b <= 'z') b -= 32;
            if (a != b) { match = false; break; }
        }

        if (match) {
            int ri = 0;
            for (int k = 0; k <= last_space; k++)
                result[ri++] = partial[k];
            for (int k = 0; dir_entries[i].name[k]; k++)
                result[ri++] = dir_entries[i].name[k];
            result[ri] = '\0';
            return result;
        }
    }
    return nullptr;
}

} // namespace re36
