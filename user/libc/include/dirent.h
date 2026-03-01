#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <sys/syscall.h> // for vfs_dir_entry

struct dirent {
    uint32_t d_ino;            // Inode number
    char d_name[13];           // Null-terminated filename (FAT16 format)
    unsigned char d_type;      // Type of file (DT_DIR, DT_REG)
    uint32_t d_size;           // File size
};

// POSIX defined file types
#define DT_UNKNOWN  0
#define DT_FIFO     1
#define DT_CHR      2
#define DT_DIR      4
#define DT_BLK      6
#define DT_REG      8
#define DT_LNK      10
#define DT_SOCK     12
#define DT_WHT      14

typedef struct {
    char* path;                  // Path string used to open the directory
    struct vfs_dir_entry* buf;   // Internal buffer of directory entries
    int max_entries;             // Allocated capacity of the buffer
    int num_entries;             // Number of valid entries returned by kernel
    int current_index;           // Current enumeration index
    struct dirent current_entry; // The POSIX struct we return to the user
} DIR;

DIR* opendir(const char* name);
struct dirent* readdir(DIR* dirp);
int closedir(DIR* dirp);

#ifdef __cplusplus
}
#endif
