#include "kernel/vfs.h"
#include "kernel/kmalloc.h"
#include "libc.h"

namespace re36 {

void vnode_release(vnode* vn) {
    if (!vn) return;
    if (__atomic_sub_fetch(&vn->refcount, 1, __ATOMIC_SEQ_CST) == 0) {
        if (vn->ops && vn->ops->close) vn->ops->close(vn);
        if (vn->fs_data) kfree(vn->fs_data);
        kfree(vn);
    }
}

void file_release(file* f) {
    if (!f) return;
    if (__atomic_sub_fetch(&f->refcount, 1, __ATOMIC_SEQ_CST) == 0) {
        vnode_release(f->vn);
        kfree(f);
    }
}

#define MAX_VFS_DRIVERS 8
#define MAX_MOUNT_POINTS 8

static vfs_filesystem_driver* fs_drivers[MAX_VFS_DRIVERS];
static int num_drivers = 0;

static superblock* mount_points[MAX_MOUNT_POINTS];
static int num_mount_points = 0;

void vfs_init() {
    for (int i = 0; i < MAX_VFS_DRIVERS; i++) fs_drivers[i] = nullptr;
    for (int i = 0; i < MAX_MOUNT_POINTS; i++) mount_points[i] = nullptr;
    num_drivers = 0;
    num_mount_points = 0;
    printf("[VFS] Initialized\n");
}

int vfs_register(vfs_filesystem_driver* driver) {
    if (!driver || !driver->name) return -1;
    if (num_drivers >= MAX_VFS_DRIVERS) return -1;

    fs_drivers[num_drivers++] = driver;
    printf("[VFS] Registered filesystem driver: %s\n", driver->name);
    return 0;
}

int vfs_mount(const char* fs_type, const char* target_path, block_device* bdev) {
    if (num_mount_points >= MAX_MOUNT_POINTS) return -1;

    vfs_filesystem_driver* driver = nullptr;
    for (int i = 0; i < num_drivers; i++) {
        const char* dname = fs_drivers[i]->name;
        int j = 0;
        bool match = true;
        while (dname[j] && fs_type[j]) {
            if (dname[j] != fs_type[j]) { match = false; break; }
            j++;
        }
        if (match && !dname[j] && !fs_type[j]) {
            driver = fs_drivers[i];
            break;
        }
    }

    if (!driver) {
        printf("[VFS] Driver not found: %s\n", fs_type);
        return -1;
    }

    superblock* sb = (superblock*)kmalloc(sizeof(superblock));
    if (!sb) return -1;

    sb->driver = driver;
    sb->bdev = bdev;
    sb->root_vnode = nullptr;

    if (driver->mount(bdev, sb) != 0) {
        printf("[VFS] Mount failed for %s on %s\n", fs_type, target_path);
        kfree(sb);
        return -1;
    }

    mount_points[num_mount_points++] = sb;
    printf("[VFS] Mounted %s on %s successfully\n", fs_type, target_path);

    return 0;
}

// Helper to extract the next path component. Returns length of component.
static int extract_path_component(const char* path, int start_idx, char* out_buf, int max_len) {
    int i = start_idx;
    int o = 0;
    while (path[i] == '/') i++; // skip extra slashes
    if (!path[i]) return 0; // end of path

    while (path[i] && path[i] != '/' && o < max_len - 1) {
        out_buf[o++] = path[i++];
    }
    out_buf[o] = '\0';
    return i - start_idx;
}

// Parses paths like /A/B/C and resolves them recursively.
int vfs_resolve_path(const char* path, vnode** out) {
    if (!path || !out) return -1;
    if (num_mount_points == 0) return -1;

    superblock* root_sb = mount_points[0]; // TODO: Support other mount points based on prefix
    if (!root_sb || !root_sb->root_vnode) return -1;

    vnode* current = root_sb->root_vnode;
    int path_idx = 0;
    char component[64];

    while (true) {
        int consumed = extract_path_component(path, path_idx, component, sizeof(component));
        if (consumed == 0) break; // Finished parsing path

        // We have a component, let's lookup in 'current' directory
        if (!current->ops || !current->ops->lookup) {
            // No lookup operation -> not a directory or not supported
            if (current != root_sb->root_vnode) vnode_release(current);
            return -1;
        }

        vnode* next_vn = nullptr;
        int res = current->ops->lookup(current, component, &next_vn);
        
        // If we allocated a temporary vnode during traversal, we should release it.
        if (current != root_sb->root_vnode) vnode_release(current);

        if (res != 0 || !next_vn) {
            return -1; // Not found
        }

        current = next_vn;
        path_idx += consumed;
    }

    // Found the target node
    if (current == root_sb->root_vnode) {
        // Return a referenced root node
        __atomic_add_fetch(&current->refcount, 1, __ATOMIC_SEQ_CST);
    }
    *out = current;
    return 0;
}

int vfs_open(const char* path, int flags, int mode) {
    (void)mode;
    vnode* vn = nullptr;
    
    int resolve_res = vfs_resolve_path(path, &vn);
    
    // Если файла нет и есть флаг O_CREAT, надо попытаться создать файл
    if (resolve_res != 0) {
        if (flags & O_CREAT) {
            // Берем корневой узел
            if (num_mount_points == 0) return -1;
            vnode* root = mount_points[0]->root_vnode;
            if (root && root->ops && root->ops->create) {
                int i = 0;
                if (path[0] == '/') i++;
                int cr = root->ops->create(root, path + i, mode, &vn);
                if (cr != 0) return -1;
            } else {
                return -1;
            }
        } else {
            return -1; // Файл не найден и мы не создаем
        }
    }

    if (!vn) return -1;

    if (vn->ops && vn->ops->open) {
        int op_res = vn->ops->open(vn);
        if (op_res != 0) return -1;
    }

    // Вместо возвращения FD тут, мы должны вернуть указатель на абстрактную структуру file*, 
    // но Syscall Gate ждет int FD. 
    // Поэтому мы вернем -2 как ошибку и позволим Syscall Gate выделить FD в `threads[cur].fd_table` 
    // и связать его с этим vnode*.
    
    // Хак: мы вернем адрес vnode*, прикастованный к int, а сисколл конвертирует его.
    // Это грязный трюк для перехода, правильнее сделать API `file* vfs_open`
    return (int)vn;
}

int vfs_readdir(const char* path, vfs_dir_entry* entries, int max_entries) {
    if (!entries || max_entries <= 0) return -1;
    if (!path) return -1;

    vnode* dir = nullptr;
    if (vfs_resolve_path(path, &dir) != 0 || !dir) return -1;

    if (!dir->ops || !dir->ops->readdir) {
        vnode_release(dir);
        return -1;
    }

    int rc = dir->ops->readdir(dir, entries, max_entries);
    vnode_release(dir);
    return rc;
}

int vfs_stat(const char* path, vfs_stat_t* out) {
    if (!path || !out) return -1;
    
    // Resolve the vnode itself
    vnode* target = nullptr;
    if (vfs_resolve_path(path, &target) != 0 || !target) return -1;

    // Fast path: if the target supports stat directly
    if (target->ops && target->ops->stat) {
        // FAT16 stat implementation currently expects a parent dir and a name.
        // We will change FAT16 stat to just take the target node, but for now we must adapt:
        // Or better yet, we can resolve the parent, and then call stat.
    }
    vnode_release(target);
    
    // Resolve parent directory and filename
    char dir_path[256];
    char filename[64];
    int len = 0;
    while (path[len]) len++;
    
    int last_slash = -1;
    for (int i = len - 1; i >= 0; i--) {
        if (path[i] == '/') {
            last_slash = i;
            break;
        }
    }

    if (last_slash == -1) {
        dir_path[0] = '/'; dir_path[1] = '\0';
        int i;
        for (i = 0; path[i] && i < 63; i++) filename[i] = path[i];
        filename[i] = '\0';
    } else {
        if (last_slash == 0) {
            dir_path[0] = '/'; dir_path[1] = '\0';
        } else {
            int i;
            for (i = 0; i < last_slash && i < 255; i++) dir_path[i] = path[i];
            dir_path[i] = '\0';
        }
        int o = 0;
        for (int i = last_slash + 1; path[i] && o < 63; i++) filename[o++] = path[i];
        filename[o] = '\0';
    }

    vnode* parent = nullptr;
    if (vfs_resolve_path(dir_path, &parent) != 0 || !parent) return -1;

    if (!parent->ops || !parent->ops->stat) {
        vnode_release(parent);
        return -1;
    }

    int rc = parent->ops->stat(parent, filename, out);
    vnode_release(parent);
    return rc;
}

int vfs_unlink(const char* path) {
    if (!path) return -1;

    // Resolve parent directory and filename
    char dir_path[256];
    char filename[64];
    int len = 0;
    while (path[len]) len++;
    
    int last_slash = -1;
    for (int i = len - 1; i >= 0; i--) {
        if (path[i] == '/') {
            last_slash = i;
            break;
        }
    }

    if (last_slash == 0 && path[1] == '\0') {
        return -1; // Cannot unlink root
    }

    if (last_slash == -1) {
        dir_path[0] = '/'; dir_path[1] = '\0';
        int i;
        for (i = 0; path[i] && i < 63; i++) filename[i] = path[i];
        filename[i] = '\0';
    } else if (last_slash == 0) {
        dir_path[0] = '/'; dir_path[1] = '\0';
    } else {
        int i;
        for (i = 0; i < last_slash && i < 255; i++) dir_path[i] = path[i];
        dir_path[i] = '\0';
    }
    int o = 0;
    for (int i = last_slash + 1; path[i] && o < 63; i++) filename[o++] = path[i];
    filename[o] = '\0';

    vnode* parent = nullptr;
    if (vfs_resolve_path(dir_path, &parent) != 0 || !parent) return -1;

    if (!parent->ops || !parent->ops->unlink) {
        vnode_release(parent);
        return -1;
    }

    int rc = parent->ops->unlink(parent, filename);
    vnode_release(parent);
    return rc;
}

int vfs_write_file(const char* path, const uint8_t* data, uint32_t size) {
    if (!path) return -1;
    if (num_mount_points == 0) return -1;

    // Extract directory path and filename
    char dir_path[256];
    char filename[64];
    int len = 0;
    while (path[len]) len++;
    
    int last_slash = -1;
    for (int i = len - 1; i >= 0; i--) {
        if (path[i] == '/') {
            last_slash = i;
            break;
        }
    }

    if (last_slash == -1) {
        dir_path[0] = '/'; dir_path[1] = '\0';
        int i;
        for (i = 0; path[i] && i < 63; i++) filename[i] = path[i];
        filename[i] = '\0';
    } else {
        if (last_slash == 0) {
            dir_path[0] = '/'; dir_path[1] = '\0';
        } else {
            int i;
            for (i = 0; i < last_slash && i < 255; i++) dir_path[i] = path[i];
            dir_path[i] = '\0';
        }
        int o = 0;
        for (int i = last_slash + 1; path[i] && o < 63; i++) {
            filename[o++] = path[i];
        }
        filename[o] = '\0';
    }

    vnode* parent = nullptr;
    if (vfs_resolve_path(dir_path, &parent) != 0 || !parent) {
        return -1;
    }

    vnode* vn = nullptr;
    if (parent->ops && parent->ops->lookup) {
        parent->ops->lookup(parent, filename, &vn);
    }

    if (!vn) {
        if (parent->ops && parent->ops->create) {
            if (parent->ops->create(parent, filename, 0, &vn) != 0) {
                vnode_release(parent);
                return -1;
            }
        } else {
            vnode_release(parent);
            return -1;
        }
    }

    vnode_release(parent);

    if (!vn || !vn->ops || !vn->ops->write) {
        if (vn) vnode_release(vn);
        return -1;
    }

    int result = vn->ops->write(vn, 0, data, size);
    vnode_release(vn);
    return result;
}

int vfs_mkdir(const char* path, int mode) {
    if (!path) return -1;
    if (num_mount_points == 0) return -1;

    // Extract directory path and new dir name
    char dir_path[256];
    char dirname[64];
    int len = 0;
    while (path[len]) len++;
    
    int last_slash = -1;
    for (int i = len - 1; i >= 0; i--) {
        if (path[i] == '/') {
            last_slash = i;
            break;
        }
    }

    if (last_slash == 0 && path[1] == '\0') {
        return -1; // Cannot mkdir root
    }

    if (last_slash == -1) {
        dir_path[0] = '/'; dir_path[1] = '\0';
        int i;
        for (i = 0; path[i] && i < 63; i++) dirname[i] = path[i];
        dirname[i] = '\0';
    } else if (last_slash == 0) {
        dir_path[0] = '/'; dir_path[1] = '\0';
    } else {
        int i;
        for (i = 0; i < last_slash && i < 255; i++) dir_path[i] = path[i];
        dir_path[i] = '\0';
    }
    int o = 0;
    for (int i = last_slash + 1; path[i] && o < 63; i++) dirname[o++] = path[i];
    dirname[o] = '\0';

    vnode* parent = nullptr;
    if (vfs_resolve_path(dir_path, &parent) != 0 || !parent) {
        return -1;
    }

    if (!parent->ops || !parent->ops->mkdir) {
        vnode_release(parent);
        return -1;
    }

    int rc = parent->ops->mkdir(parent, dirname, mode);
    vnode_release(parent);
    return rc;
}

int vfs_rename(const char* oldpath, const char* newpath) {
    if (!oldpath || !newpath) return -1;
    if (num_mount_points == 0) return -1;

    // Parse oldpath
    char old_dir_path[256];
    char old_filename[64];
    int len = 0; while (oldpath[len]) len++;
    int last_slash = -1;
    for (int i = len - 1; i >= 0; i--) {
        if (oldpath[i] == '/') { last_slash = i; break; }
    }
    if (last_slash == 0 && oldpath[1] == '\0') return -1; // Cannot rename root
    if (last_slash == -1) {
        old_dir_path[0] = '/'; old_dir_path[1] = '\0';
        for (int i=0; oldpath[i] && i<63; i++) old_filename[i] = oldpath[i]; old_filename[63]='\0';
    } else if (last_slash == 0) {
        old_dir_path[0] = '/'; old_dir_path[1] = '\0';
    } else {
        int i; for (i = 0; i < last_slash && i < 255; i++) old_dir_path[i] = oldpath[i];
        old_dir_path[i] = '\0';
    }
    if (last_slash != -1) {
        int o = 0; for (int i = last_slash + 1; oldpath[i] && o < 63; i++) old_filename[o++] = oldpath[i];
        old_filename[o] = '\0';
    }

    // Parse newpath
    char new_dir_path[256];
    char new_filename[64];
    len = 0; while (newpath[len]) len++;
    last_slash = -1;
    for (int i = len - 1; i >= 0; i--) {
        if (newpath[i] == '/') { last_slash = i; break; }
    }
    if (last_slash == 0 && newpath[1] == '\0') return -1;
    if (last_slash == -1) {
        new_dir_path[0] = '/'; new_dir_path[1] = '\0';
        for (int i=0; newpath[i] && i<63; i++) new_filename[i] = newpath[i]; new_filename[63]='\0';
    } else if (last_slash == 0) {
        new_dir_path[0] = '/'; new_dir_path[1] = '\0';
    } else {
        int i; for (i = 0; i < last_slash && i < 255; i++) new_dir_path[i] = newpath[i];
        new_dir_path[i] = '\0';
    }
    if (last_slash != -1) {
        int o = 0; for (int i = last_slash + 1; newpath[i] && o < 63; i++) new_filename[o++] = newpath[i];
        new_filename[o] = '\0';
    }

    vnode* old_parent = nullptr;
    if (vfs_resolve_path(old_dir_path, &old_parent) != 0 || !old_parent) return -1;

    vnode* new_parent = nullptr;
    if (vfs_resolve_path(new_dir_path, &new_parent) != 0 || !new_parent) {
        vnode_release(old_parent);
        return -1;
    }

    int rc = -1;
    if (old_parent->ops && old_parent->ops->rename) {
        rc = old_parent->ops->rename(old_parent, old_filename, new_parent, new_filename);
    }

    vnode_release(old_parent);
    vnode_release(new_parent);
    return rc;
}

} // namespace re36
