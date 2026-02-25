#include "kernel/shell_autocomplete.h"
#include "kernel/shell_history.h"
#include "kernel/fat16.h"
#include "kernel/ata.h"
#include "libc.h"

namespace re36 {

static const char* builtin_cmds[] = {
    "cat", "clear", "date", "exec", "hello", "help", "hexdump",
    "ls", "meminfo", "ps", "ring3", "rm", "stat", "syscall",
    "ticks", "write", nullptr
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
    if (Fat16::is_mounted()) {
        uint8_t dir_buf[512];
        for (uint32_t s = 0; s < 14; s++) {
            if (!ATA::read_sectors(Fat16::root_dir_lba() + s, 1, dir_buf)) continue;
            FAT16_DirEntry* entries = (FAT16_DirEntry*)dir_buf;
            for (int i = 0; i < 16; i++) {
                if (entries[i].name[0] == 0x00) return nullptr;
                if ((uint8_t)entries[i].name[0] == 0xE5) continue;
                if (entries[i].attributes & (FAT_ATTR_LFN | FAT_ATTR_VOLUME_ID)) continue;

                char fname[13];
                int ci = 0;
                for (int j = 0; j < 8 && entries[i].name[j] != ' '; j++)
                    fname[ci++] = entries[i].name[j];
                if (entries[i].ext[0] != ' ') {
                    fname[ci++] = '.';
                    for (int j = 0; j < 3 && entries[i].ext[j] != ' '; j++)
                        fname[ci++] = entries[i].ext[j];
                }
                fname[ci] = '\0';

                bool match = true;
                for (int k = 0; k < flen; k++) {
                    char a = file_part[k];
                    char b = fname[k];
                    if (a >= 'a' && a <= 'z') a -= 32;
                    if (b >= 'a' && b <= 'z') b -= 32;
                    if (a != b) { match = false; break; }
                }

                if (match) {
                    int ri = 0;
                    for (int k = 0; k <= last_space; k++)
                        result[ri++] = partial[k];
                    for (int k = 0; fname[k]; k++)
                        result[ri++] = fname[k];
                    result[ri] = '\0';
                    return result;
                }
            }
        }
    }
    return nullptr;
}

} // namespace re36
