#pragma once

#include <stdint.h>
#include <stddef.h>

namespace re36 {

struct vnode;
struct superblock;
struct block_device; // Forward declaration, will resolve to ATA/Disk later

// Vnode types
enum class VnodeType {
    File,
    Directory,
    Device,
    Symlink,
    MountPoint
};

struct vfs_dir_entry {
    char name[13];
    uint32_t size;
    uint8_t type;
};

#define VFS_DIR_MAX_ENTRIES 64

struct vfs_stat_t {
    uint32_t size;
    VnodeType type;
    uint16_t first_cluster;
    uint16_t mod_time;
    uint16_t mod_date;
    uint8_t attributes;
};

// Open file flags
#define O_RDONLY    0x0000
#define O_WRONLY    0x0001
#define O_RDWR      0x0002
#define O_CREAT     0x0040
#define O_EXCL      0x0080
#define O_TRUNC     0x0200
#define O_APPEND    0x0400
#define O_CLOEXEC   0x0800

// Vnode operations table (implemented by specific FS driver like FAT16)
struct vnode_operations {
    int (*open)(vnode* vn);
    int (*close)(vnode* vn);
    int (*read)(vnode* vn, uint32_t offset, uint8_t* buffer, uint32_t size);
    int (*write)(vnode* vn, uint32_t offset, const uint8_t* buffer, uint32_t size);
    int (*lookup)(vnode* dir, const char* name, vnode** out);
    int (*create)(vnode* dir, const char* name, int mode, vnode** out);
    int (*mkdir)(vnode* dir, const char* name, int mode);
    int (*unlink)(vnode* dir, const char* name);
    int (*readdir)(vnode* dir, vfs_dir_entry* entries, int max_entries);
    int (*stat)(vnode* dir, const char* name, vfs_stat_t* out);
};

// Abstract representation of a file/directory
struct vnode {
    VnodeType type;
    uint32_t size;
    uint32_t inode_num;      // Underlying FS inode/cluster number
    uint32_t refcount;       // Use count for memory management
    
    superblock* sb;          // Superblock this vnode belongs to
    vnode_operations* ops;   // FS specific operations
    vnode* mount_target;     // If this is a mount point, points to root vnode of mounted FS
    
    void* fs_data;           // FS specific private data
};

// FS Driver Plugin Interface
struct vfs_filesystem_driver {
    const char* name;
    int (*mount)(block_device* bdev, superblock* sb);
    int (*unmount)(superblock* sb);
};

// Superblock representing a mounted filesystem
struct superblock {
    struct vfs_filesystem_driver* driver;
    block_device* bdev;
    vnode* root_vnode;
    void* fs_data;           // Superblock specific FS data
};



// File descriptor object representing an open file per process
struct file {
    vnode* vn;
    uint32_t offset;
    uint32_t flags;
    uint32_t refcount;
};

void vnode_release(vnode* vn);
void file_release(file* f);

void vfs_init();
int vfs_register(vfs_filesystem_driver* driver);
int vfs_mount(const char* fs_type, const char* target_path, block_device* bdev);

int vfs_open(const char* path, int flags, int mode);

int vfs_resolve_path(const char* path, vnode** out);
int vfs_readdir(const char* path, vfs_dir_entry* entries, int max_entries);
int vfs_stat(const char* path, vfs_stat_t* out);
int vfs_unlink(const char* path);
int vfs_write_file(const char* path, const uint8_t* data, uint32_t size);

} // namespace re36
