#include "kernel/vfs.h"
#include "kernel/kmalloc.h"
#include "libc.h"

namespace re36 {

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

// Упрощенный Path Resolver: 
// В этой минимальной реализации мы считаем, что мы всегда обращаемся к корню первой смонтированной ФС (FAT16).
// Для поддержки /mnt/disk и директорий потребуется полноценный парсер путей и реализация lookup.
int vfs_resolve_path(const char* path, vnode** out) {
    if (!path || !out) return -1;
    if (num_mount_points == 0) return -1;

    superblock* root_sb = mount_points[0]; // Пока берем первую ФС
    if (!root_sb || !root_sb->root_vnode) return -1;

    vnode* current = root_sb->root_vnode;

    // Временный хак: пропускаем слеш "/"
    int i = 0;
    if (path[0] == '/') i++;

    // Вызываем lookup драйвера для поиска файла
    if (current->ops && current->ops->lookup) {
        return current->ops->lookup(current, path + i, out);
    }

    return -1;
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

int vfs_read(int fd, uint8_t* buffer, uint32_t size) {
    // В VFS API мы работаем с `struct file`, поэтому мы передаем сюда распакованные параметры
    // из `syscall_gate`. `fd` - это просто идентификатор, нам нужен `file*`.
    // Истинная реализация будет в syscall_gate->vfs_read_internal(file* f, buf, size).
    (void)fd; (void)buffer; (void)size;
    return -1;
}

int vfs_write(int fd, const uint8_t* buffer, uint32_t size) {
    (void)fd; (void)buffer; (void)size;
    return -1;
}

int vfs_close(int fd) {
    (void)fd;
    return -1;
}

} // namespace re36
