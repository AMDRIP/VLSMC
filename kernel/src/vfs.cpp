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

static bool split_parent_and_name(const char* path, char* dir_out, int dir_max, char* name_out, int name_max);
static int vfs_resolve_path_internal(const char* path, vnode** out, int depth);
static int vfs_stat_internal(const char* path, vfs_stat_t* out, int depth);

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

static void copy_path_range(const char* src, int start, int end, char* out, int max) {
    int o = 0;
    for (int i = start; i < end && o < max - 1; i++) {
        out[o++] = src[i];
    }
    out[o] = '\0';
}

static int append_str(char* out, int pos, int max, const char* str) {
    int i = 0;
    while (str && str[i] && pos < max - 1) {
        out[pos++] = str[i++];
    }
    out[pos] = '\0';
    return pos;
}

static void build_symlink_follow_path(const char* path, int component_start, const char* target, const char* rest, char* out, int max) {
    int pos = 0;

    if (target && target[0] == '/') {
        pos = append_str(out, pos, max, target);
    } else {
        if (component_start <= 0) {
            out[pos++] = '/';
            out[pos] = '\0';
        } else {
            copy_path_range(path, 0, component_start, out, max);
            while (out[pos]) pos++;
            if (pos == 0) {
                out[pos++] = '/';
                out[pos] = '\0';
            }
        }
        pos = append_str(out, pos, max, target);
    }

    if (rest && rest[0]) {
        if (pos > 1 && out[pos - 1] == '/' && rest[0] == '/') {
            rest++;
        } else if (pos > 0 && out[pos - 1] != '/' && rest[0] != '/' && pos < max - 1) {
            out[pos++] = '/';
            out[pos] = '\0';
        }
        append_str(out, pos, max, rest);
    }
}

static bool split_parent_and_name(const char* path, char* dir_out, int dir_max, char* name_out, int name_max) {
    if (!path || !dir_out || !name_out || dir_max < 2 || name_max < 2) return false;

    int len = 0;
    while (path[len]) len++;

    // Trim trailing slashes but preserve root ("/").
    while (len > 1 && path[len - 1] == '/') len--;

    if (len <= 0) return false;
    if (len == 1 && path[0] == '/') return false; // root has no name component

    int last_slash = -1;
    for (int i = len - 1; i >= 0; i--) {
        if (path[i] == '/') {
            last_slash = i;
            break;
        }
    }

    if (last_slash <= 0) {
        dir_out[0] = '/';
        dir_out[1] = '\0';
    } else {
        int di = 0;
        while (di < last_slash && di < dir_max - 1) {
            dir_out[di] = path[di];
            di++;
        }
        dir_out[di] = '\0';
    }

    int ni = 0;
    for (int i = last_slash + 1; i < len && ni < name_max - 1; i++) {
        name_out[ni++] = path[i];
    }
    name_out[ni] = '\0';

    return name_out[0] != '\0';
}

// Parses paths like /A/B/C and resolves them recursively.
int vfs_resolve_path(const char* path, vnode** out) {
    return vfs_resolve_path_internal(path, out, 0);
}

static int vfs_resolve_path_internal(const char* path, vnode** out, int depth) {
    if (!path || !out) return -1;
    if (num_mount_points == 0) return -1;
    if (depth > 8) return -1;

    superblock* root_sb = mount_points[0]; // TODO: Support other mount points based on prefix
    if (!root_sb || !root_sb->root_vnode) return -1;

    vnode* current = root_sb->root_vnode;
    int path_idx = 0;
    char component[64];

    while (true) {
        int component_start = path_idx;
        while (path[component_start] == '/') component_start++;
        if (!path[component_start]) break;

        int component_end = component_start;
        int ci = 0;
        while (path[component_end] && path[component_end] != '/') {
            if (ci < (int)sizeof(component) - 1) component[ci++] = path[component_end];
            component_end++;
        }
        component[ci] = '\0';

        // We have a component, let's lookup in 'current' directory
        if (!current->ops || !current->ops->lookup) {
            // No lookup operation -> not a directory or not supported
            if (current != root_sb->root_vnode) vnode_release(current);
            return -1;
        }

        vnode* next_vn = nullptr;
        int res = current->ops->lookup(current, component, &next_vn);

        if (res != 0 || !next_vn) {
            if (current != root_sb->root_vnode) vnode_release(current);
            return -1; // Not found
        }

        if (next_vn->type == VnodeType::Symlink) {
            char target[256];
            if (!current->ops || !current->ops->readlink ||
                current->ops->readlink(current, component, target, sizeof(target)) < 0) {
                if (current != root_sb->root_vnode) vnode_release(current);
                vnode_release(next_vn);
                return -1;
            }

            char follow_path[512];
            build_symlink_follow_path(path, component_start, target, &path[component_end], follow_path, sizeof(follow_path));

            if (current != root_sb->root_vnode) vnode_release(current);
            vnode_release(next_vn);
            return vfs_resolve_path_internal(follow_path, out, depth + 1);
        }

        // If we allocated a temporary vnode during traversal, we should release it.
        if (current != root_sb->root_vnode) vnode_release(current);
        current = next_vn;
        path_idx = component_end;
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
    if (!path) return -1;
    vnode* vn = nullptr;
    
    int resolve_res = vfs_resolve_path(path, &vn);
    
    // Если файла нет и есть флаг O_CREAT, надо попытаться создать файл
    if (resolve_res != 0) {
        if (flags & O_CREAT) {
            char parent_path[256];
            char filename[64];
            if (!split_parent_and_name(path, parent_path, sizeof(parent_path), filename, sizeof(filename))) {
                return -1;
            }

            vnode* parent = nullptr;
            if (vfs_resolve_path(parent_path, &parent) != 0 || !parent) {
                return -1;
            }

            if (parent->ops && parent->ops->create) {
                int cr = parent->ops->create(parent, filename, mode, &vn);
                vnode_release(parent);
                if (cr != 0) return -1;
            } else {
                vnode_release(parent);
                return -1;
            }
        } else {
            return -1; // Файл не найден и мы не создаем
        }
    } else if ((flags & O_CREAT) && (flags & O_EXCL)) {
        vnode_release(vn);
        return -1;
    }

    if (!vn) return -1;

    if (vn->ops && vn->ops->open) {
        int op_res = vn->ops->open(vn);
        if (op_res != 0) {
            vnode_release(vn);
            return -1;
        }
    }

    if ((flags & O_TRUNC) && (flags & (O_WRONLY | O_RDWR))) {
        if (!vn->ops || !vn->ops->write || vn->ops->write(vn, 0, nullptr, 0) < 0) {
            vnode_release(vn);
            return -1;
        }
        vn->size = 0;
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

int vfs_lstat(const char* path, vfs_stat_t* out) {
    if (!path || !out) return -1;

    if (path[0] == '/' && path[1] == '\0') {
        vnode* root = nullptr;
        if (vfs_resolve_path("/", &root) != 0 || !root) return -1;
        out->size = root->size;
        out->type = root->type;
        out->nlink = 1;
        out->first_cluster = (uint16_t)root->inode_num;
        out->mod_time = 0;
        out->mod_date = 0;
        out->attributes = 0x10;
        vnode_release(root);
        return 0;
    }

    char dir_path[256];
    char filename[64];
    if (!split_parent_and_name(path, dir_path, sizeof(dir_path), filename, sizeof(filename))) {
        return -1;
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

int vfs_stat(const char* path, vfs_stat_t* out) {
    return vfs_stat_internal(path, out, 0);
}

static int vfs_stat_internal(const char* path, vfs_stat_t* out, int depth) {
    if (!path || !out || depth > 8) return -1;

    if (vfs_lstat(path, out) != 0) return -1;
    if (out->type != VnodeType::Symlink) return 0;

    char target[256];
    int r = vfs_readlink(path, target, sizeof(target));
    if (r < 0) return -1;

    int len = 0;
    while (path[len]) len++;
    int last_slash = -1;
    for (int i = len - 1; i >= 0; i--) {
        if (path[i] == '/') { last_slash = i; break; }
    }

    char follow_path[512];
    build_symlink_follow_path(path, last_slash + 1, target, "", follow_path, sizeof(follow_path));
    return vfs_stat_internal(follow_path, out, depth + 1);
}

int vfs_unlink(const char* path) {
    if (!path) return -1;

    char dir_path[256];
    char filename[64];
    if (!split_parent_and_name(path, dir_path, sizeof(dir_path), filename, sizeof(filename))) {
        return -1;
    }

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

    char dir_path[256];
    char filename[64];
    if (!split_parent_and_name(path, dir_path, sizeof(dir_path), filename, sizeof(filename))) {
        return -1;
    }

    vnode* parent = nullptr;
    if (vfs_resolve_path(dir_path, &parent) != 0 || !parent) {
        return -1;
    }

    vnode* vn = nullptr;
    if (parent->ops && parent->ops->lookup) {
        parent->ops->lookup(parent, filename, &vn);
    }

    if (vn && vn->type == VnodeType::Symlink) {
        vnode_release(parent);
        vnode_release(vn);
        vnode* target = nullptr;
        if (vfs_resolve_path(path, &target) != 0 || !target) return -1;
        if (!target->ops || !target->ops->write) {
            vnode_release(target);
            return -1;
        }
        int result = target->ops->write(target, 0, data, size);
        vnode_release(target);
        return result;
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

    char dir_path[256];
    char dirname[64];
    if (!split_parent_and_name(path, dir_path, sizeof(dir_path), dirname, sizeof(dirname))) {
        return -1;
    }

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

    char old_dir_path[256];
    char old_filename[64];
    if (!split_parent_and_name(oldpath, old_dir_path, 256, old_filename, 64)) return -1;

    char new_dir_path[256];
    char new_filename[64];
    if (!split_parent_and_name(newpath, new_dir_path, 256, new_filename, 64)) return -1;

    if (old_filename[0] == '\0' || new_filename[0] == '\0') return -1;

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

int vfs_link(const char* oldpath, const char* newpath) {
    if (!oldpath || !newpath) return -1;
    if (num_mount_points == 0) return -1;

    char old_dir_path[256];
    char old_filename[64];
    split_parent_and_name(oldpath, old_dir_path, sizeof(old_dir_path), old_filename, sizeof(old_filename));

    char new_dir_path[256];
    char new_filename[64];
    split_parent_and_name(newpath, new_dir_path, sizeof(new_dir_path), new_filename, sizeof(new_filename));

    if (old_filename[0] == '\0' || new_filename[0] == '\0') return -1;

    vnode* old_parent = nullptr;
    if (vfs_resolve_path(old_dir_path, &old_parent) != 0 || !old_parent) return -1;

    vnode* new_parent = nullptr;
    if (vfs_resolve_path(new_dir_path, &new_parent) != 0 || !new_parent) {
        vnode_release(old_parent);
        return -1;
    }

    int rc = -1;
    if (old_parent->ops && old_parent->ops == new_parent->ops && old_parent->ops->link) {
        rc = old_parent->ops->link(old_parent, old_filename, new_parent, new_filename);
    }

    vnode_release(old_parent);
    vnode_release(new_parent);
    return rc;
}

int vfs_symlink(const char* target, const char* linkpath) {
    if (!target || !linkpath) return -1;
    if (num_mount_points == 0) return -1;

    char dir_path[256];
    char filename[64];
    split_parent_and_name(linkpath, dir_path, sizeof(dir_path), filename, sizeof(filename));
    if (filename[0] == '\0') return -1;

    vnode* parent = nullptr;
    if (vfs_resolve_path(dir_path, &parent) != 0 || !parent) return -1;

    int rc = -1;
    if (parent->ops && parent->ops->symlink) {
        rc = parent->ops->symlink(parent, filename, target);
    }

    vnode_release(parent);
    return rc;
}

int vfs_readlink(const char* path, char* buffer, uint32_t size) {
    if (!path || !buffer || size == 0) return -1;

    char dir_path[256];
    char filename[64];
    split_parent_and_name(path, dir_path, sizeof(dir_path), filename, sizeof(filename));
    if (filename[0] == '\0') return -1;

    vnode* parent = nullptr;
    if (vfs_resolve_path(dir_path, &parent) != 0 || !parent) return -1;

    int rc = -1;
    if (parent->ops && parent->ops->readlink) {
        rc = parent->ops->readlink(parent, filename, buffer, size);
    }

    vnode_release(parent);
    return rc;
}

} // namespace re36
