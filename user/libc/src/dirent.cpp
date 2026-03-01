#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include "../../syscalls.h"

extern "C" {

DIR* opendir(const char* name) {
    if (!name) return nullptr;
    
    DIR* d = (DIR*)malloc(sizeof(DIR));
    if (!d) return nullptr;
    
    int len = strlen(name);
    d->path = (char*)malloc(len + 1);
    if (!d->path) {
        free(d);
        return nullptr;
    }
    strcpy(d->path, name);
    
    // Allocate 64 entries max initially, VLSMC usually limits to 64
    d->max_entries = 64;
    d->buf = (struct vfs_dir_entry*)malloc(sizeof(struct vfs_dir_entry) * d->max_entries);
    if (!d->buf) {
        free(d->path);
        free(d);
        return nullptr;
    }
    
    // Initial fetch
    d->num_entries = sys_readdir(d->path, d->buf, d->max_entries);
    d->current_index = 0;
    
    if (d->num_entries < 0) {
        // Path doesn't exist or isn't a directory
        free(d->buf);
        free(d->path);
        free(d);
        return nullptr;
    }
    
    return d;
}

struct dirent* readdir(DIR* dirp) {
    if (!dirp || !dirp->buf) return nullptr;
    if (dirp->current_index >= dirp->num_entries) return nullptr; // End of directory
    
    struct vfs_dir_entry* kern_entry = &dirp->buf[dirp->current_index];
    
    // Map kernel entry to POSIX dirent
    // FAT16 names might be padded, so copy cautiously
    memset(&dirp->current_entry, 0, sizeof(struct dirent));
    for (int i = 0; i < 12; i++) {
        dirp->current_entry.d_name[i] = kern_entry->name[i];
    }
    dirp->current_entry.d_name[12] = '\0';
    
    dirp->current_entry.d_ino = dirp->current_index + 1; // Fake inode
    dirp->current_entry.d_size = kern_entry->size;
    
    // Approximate types based on standard attributes (0x10 = DIRECTORY)
    if (kern_entry->attributes & 0x10) {
        dirp->current_entry.d_type = DT_DIR;
    } else {
        dirp->current_entry.d_type = DT_REG;
    }
    
    dirp->current_index++;
    return &dirp->current_entry;
}

int closedir(DIR* dirp) {
    if (!dirp) return -1;
    
    if (dirp->buf) free(dirp->buf);
    if (dirp->path) free(dirp->path);
    free(dirp);
    
    return 0;
}

} // extern "C"
