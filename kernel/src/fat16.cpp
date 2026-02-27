#include "kernel/fat16.h"
#include "kernel/disk.h"
#include "kernel/rtc.h"
#include "kernel/pmm.h"
#include "kernel/kmalloc.h"
#include "libc.h"

namespace re36 {

FAT16_BPB Fat16::bpb_;
uint16_t Fat16::fat_table_[FAT16_MAX_FAT_ENTRIES];
uint32_t Fat16::fat_start_lba_ = 0;
uint32_t Fat16::root_dir_lba_ = 0;
uint32_t Fat16::data_start_lba_ = 0;
uint32_t Fat16::root_dir_sectors_ = 0;
bool Fat16::mounted_ = false;
uint8_t Fat16::sector_cache_[FAT16_SECTOR_BUF_SIZE] __attribute__((aligned(4096)));
uint32_t Fat16::cached_sector_ = 0xFFFFFFFF;
uint8_t* Fat16::dma_buffer_ = nullptr;

bool Fat16::read_cached_sector(uint32_t lba) {
    if (lba == cached_sector_) return true;
    if (!Disk::read_sectors(lba, 1, sector_cache_)) return false;
    cached_sector_ = lba;
    return true;
}

uint32_t Fat16::cluster_to_lba(uint16_t cluster) {
    return data_start_lba_ + ((uint32_t)(cluster - 2) * bpb_.sectors_per_cluster);
}

bool Fat16::match_filename(const FAT16_DirEntry* entry, const char* name) {
    char fat_name[12];
    int i;
    for (i = 0; i < 8; i++) fat_name[i] = entry->name[i];
    fat_name[8] = '.';
    for (i = 0; i < 3; i++) fat_name[9 + i] = entry->ext[i];
    fat_name[12] = '\0';

    int fi = 0, ni = 0;
    while (fat_name[fi] == ' ') fi++;
    
    char clean_fat[13];
    int ci = 0;
    
    for (i = 0; i < 8 && entry->name[i] != ' '; i++)
        clean_fat[ci++] = entry->name[i];
    
    if (entry->ext[0] != ' ') {
        clean_fat[ci++] = '.';
        for (i = 0; i < 3 && entry->ext[i] != ' '; i++)
            clean_fat[ci++] = entry->ext[i];
    }
    clean_fat[ci] = '\0';

    ni = 0;
    fi = 0;
    while (clean_fat[fi] && name[ni]) {
        char a = clean_fat[fi];
        char b = name[ni];
        if (a >= 'a' && a <= 'z') a -= 32;
        if (b >= 'a' && b <= 'z') b -= 32;
        if (a != b) return false;
        fi++;
        ni++;
    }
    return clean_fat[fi] == '\0' && name[ni] == '\0';
}

bool Fat16::init() {
    if (!Disk::is_present()) {
        printf("[FAT16] No ATA disk found\n");
        return false;
    }

    if (!dma_buffer_) dma_buffer_ = (uint8_t*)PhysicalMemoryManager::alloc_frame();
    if (!dma_buffer_) return false;

    if (!Disk::read_sectors(0, 1, dma_buffer_)) {
        printf("[FAT16] Failed to read boot sector\n");
        return false;
    }

    for (uint32_t i = 0; i < sizeof(FAT16_BPB); i++) {
        ((uint8_t*)&bpb_)[i] = dma_buffer_[i];
    }

    if (bpb_.bytes_per_sector != 512) {
        printf("[FAT16] Unsupported sector size: %d\n", bpb_.bytes_per_sector);
        return false;
    }

    fat_start_lba_ = bpb_.reserved_sectors;
    root_dir_lba_ = fat_start_lba_ + (bpb_.num_fats * bpb_.fat_size_16);
    root_dir_sectors_ = ((bpb_.root_entry_count * 32) + (bpb_.bytes_per_sector - 1)) / bpb_.bytes_per_sector;
    data_start_lba_ = root_dir_lba_ + root_dir_sectors_;

    uint32_t fat_sectors = bpb_.fat_size_16;
    uint32_t fat_entries = (fat_sectors * 512) / 2;
    if (fat_entries > FAT16_MAX_FAT_ENTRIES) fat_entries = FAT16_MAX_FAT_ENTRIES;

    uint32_t entry_idx = 0;
    for (uint32_t s = 0; s < fat_sectors && entry_idx < fat_entries; s++) {
        if (!Disk::read_sectors(fat_start_lba_ + s, 1, dma_buffer_)) {
            printf("[FAT16] Failed to read FAT sector %d\n", s);
            return false;
        }
        for (uint32_t i = 0; i < 256 && entry_idx < fat_entries; i++) {
            fat_table_[entry_idx++] = ((uint16_t*)dma_buffer_)[i];
        }
    }

    uint32_t total_sectors = bpb_.total_sectors_16 ? bpb_.total_sectors_16 : bpb_.total_sectors_32;
    uint32_t data_sectors = total_sectors - data_start_lba_;
    uint32_t total_clusters = data_sectors / bpb_.sectors_per_cluster;
    (void)total_clusters;

    mounted_ = true;

    printf("[FAT16] Mounted: %d sectors, %d bytes/cluster, FAT at LBA %d, Data at LBA %d\n",
        total_sectors,
        bpb_.sectors_per_cluster * 512,
        fat_start_lba_,
        data_start_lba_);

    return true;
}

void Fat16::list_root() {
    if (!mounted_) {
        printf("No filesystem mounted\n");
        return;
    }

    printf("\n  Name          Size     Cluster\n");
    printf("  ------------- -------- -------\n");

    int file_count = 0;

    for (uint32_t s = 0; s < root_dir_sectors_; s++) {
        if (!Disk::read_sectors(root_dir_lba_ + s, 1, dma_buffer_)) continue;

        FAT16_DirEntry* entries = (FAT16_DirEntry*)dma_buffer_;
        int entries_per_sector = 512 / sizeof(FAT16_DirEntry);

        for (int i = 0; i < entries_per_sector; i++) {
            if (entries[i].name[0] == 0x00) goto done;
            if ((uint8_t)entries[i].name[0] == 0xE5) continue;
            if (entries[i].attributes & FAT_ATTR_LFN) continue;
            if (entries[i].attributes & FAT_ATTR_VOLUME_ID) continue;

            char name_buf[13];
            int ci = 0;
            for (int j = 0; j < 8 && entries[i].name[j] != ' '; j++)
                name_buf[ci++] = entries[i].name[j];
            if (entries[i].ext[0] != ' ') {
                name_buf[ci++] = '.';
                for (int j = 0; j < 3 && entries[i].ext[j] != ' '; j++)
                    name_buf[ci++] = entries[i].ext[j];
            }
            name_buf[ci] = '\0';

            char type = (entries[i].attributes & FAT_ATTR_DIRECTORY) ? 'D' : 'F';
            printf("  [%c] %s\t%d B\t#%d\n",
                type, name_buf, entries[i].file_size, entries[i].first_cluster);
            file_count++;
        }
    }
done:
    printf("\n  Total: %d entries\n\n", file_count);
}

int Fat16::read_file(const char* name, uint8_t* buffer, uint32_t max_size) {
    if (!mounted_) return -1;

    FAT16_DirEntry found_entry;
    bool found = false;

    for (uint32_t s = 0; s < root_dir_sectors_; s++) {
        if (!Disk::read_sectors(root_dir_lba_ + s, 1, dma_buffer_)) continue;

        FAT16_DirEntry* entries = (FAT16_DirEntry*)dma_buffer_;
        int entries_per_sector = 512 / sizeof(FAT16_DirEntry);

        for (int i = 0; i < entries_per_sector; i++) {
            if (entries[i].name[0] == 0x00) goto not_found;
            if ((uint8_t)entries[i].name[0] == 0xE5) continue;
            if (entries[i].attributes & (FAT_ATTR_LFN | FAT_ATTR_VOLUME_ID)) continue;

            if (match_filename(&entries[i], name)) {
                found_entry = entries[i];
                found = true;
                goto read_data;
            }
        }
    }

not_found:
    return -1;

read_data:
    if (!found) return -1;

    uint32_t bytes_read = 0;
    uint16_t cluster = found_entry.first_cluster;

    while (cluster >= 2 && cluster < 0xFFF8 && bytes_read < max_size) {
        uint32_t lba = cluster_to_lba(cluster);

        for (uint8_t s = 0; s < bpb_.sectors_per_cluster; s++) {
            if (bytes_read >= max_size) break;
            if (bytes_read >= found_entry.file_size) break;

            if (!Disk::read_sectors(lba + s, 1, dma_buffer_)) return bytes_read;

            uint32_t to_copy = 512;
            if (bytes_read + to_copy > found_entry.file_size)
                to_copy = found_entry.file_size - bytes_read;
            if (bytes_read + to_copy > max_size)
                to_copy = max_size - bytes_read;

            for (uint32_t b = 0; b < to_copy; b++) {
                buffer[bytes_read + b] = dma_buffer_[b];
            }
            bytes_read += to_copy;
        }

        cluster = fat_table_[cluster];
    }

    return bytes_read;
}

int Fat16::read_file_offset(const char* name, uint32_t offset, uint8_t* buffer, uint32_t size) {
    if (!mounted_) return -1;

    uint32_t sector;
    int index;
    if (find_dir_entry(name, &sector, &index) != 0) return -1;

    Disk::read_sectors(sector, 1, dma_buffer_);
    FAT16_DirEntry* entry = &((FAT16_DirEntry*)dma_buffer_)[index];

    if (offset >= entry->file_size) return 0; // чтение за пределами файла
    if (offset + size > entry->file_size) {
        size = entry->file_size - offset; // ограничим размер до конца файла
    }

    uint16_t cluster = entry->first_cluster;
    uint32_t cluster_size = bpb_.sectors_per_cluster * 512;

    // Пропускаем ненужные кластеры
    uint32_t clusters_to_skip = offset / cluster_size;
    for (uint32_t i = 0; i < clusters_to_skip; i++) {
        if (cluster < 2 || cluster >= 0xFFF8) return -1; // Цепочка неожиданно оборвалась
        cluster = fat_table_[cluster];
    }

    uint32_t offset_in_cluster = offset % cluster_size;
    uint32_t bytes_read = 0;

    while (cluster >= 2 && cluster < 0xFFF8 && bytes_read < size) {
        uint32_t lba = cluster_to_lba(cluster);
        uint32_t sector_offset = offset_in_cluster / 512;
        uint32_t offset_in_sector = offset_in_cluster % 512;

        for (uint8_t s = sector_offset; s < bpb_.sectors_per_cluster; s++) {
            if (bytes_read >= size) break;

            if (!Disk::read_sectors(lba + s, 1, dma_buffer_)) {
                return bytes_read;
            }

            uint32_t to_copy = 512 - offset_in_sector;
            if (bytes_read + to_copy > size) {
                to_copy = size - bytes_read;
            }

            for (uint32_t b = 0; b < to_copy; b++) {
                buffer[bytes_read + b] = dma_buffer_[offset_in_sector + b];
            }

            bytes_read += to_copy;
            offset_in_sector = 0; // Смещение применяется только к первому прочитанному сектору
        }
        
        offset_in_cluster = 0; // Смещение применяется только к первому кластеру
        cluster = fat_table_[cluster];
    }

    return bytes_read;
}

bool Fat16::is_mounted() {
    return mounted_;
}

void Fat16::format_83_name(const char* name, char* out) {
    for (int i = 0; i < 11; i++) out[i] = ' ';
    
    int i = 0, o = 0;
    while (name[i] && name[i] != '.' && o < 8) {
        char c = name[i];
        if (c >= 'a' && c <= 'z') c -= 32;
        out[o++] = c;
        i++;
    }
    
    if (name[i] == '.') {
        i++;
        o = 8;
        while (name[i] && o < 11) {
            char c = name[i];
            if (c >= 'a' && c <= 'z') c -= 32;
            out[o++] = c;
            i++;
        }
    }
}

int Fat16::find_dir_entry(const char* name, uint32_t* sector_out, int* index_out) {
    if (!mounted_) return -1;
    
    for (uint32_t s = 0; s < root_dir_sectors_; s++) {
        if (!Disk::read_sectors(root_dir_lba_ + s, 1, dma_buffer_)) continue;
        
        FAT16_DirEntry* entries = (FAT16_DirEntry*)dma_buffer_;
        int entries_per_sector = 512 / sizeof(FAT16_DirEntry);
        
        for (int i = 0; i < entries_per_sector; i++) {
            if (entries[i].name[0] == 0x00) return -1;
            if ((uint8_t)entries[i].name[0] == 0xE5) continue;
            if (entries[i].attributes & (FAT_ATTR_LFN | FAT_ATTR_VOLUME_ID)) continue;
            
            if (match_filename(&entries[i], name)) {
                if (sector_out) *sector_out = root_dir_lba_ + s;
                if (index_out) *index_out = i;
                return 0;
            }
        }
    }
    return -1;
}

uint16_t Fat16::alloc_cluster() {
    uint32_t total = (bpb_.fat_size_16 * 512) / 2;
    if (total > FAT16_MAX_FAT_ENTRIES) total = FAT16_MAX_FAT_ENTRIES;
    
    for (uint32_t i = 2; i < total; i++) {
        if (fat_table_[i] == 0x0000) {
            fat_table_[i] = 0xFFFF;
            return (uint16_t)i;
        }
    }
    return 0;
}

void Fat16::free_chain(uint16_t start_cluster) {
    uint16_t cluster = start_cluster;
    while (cluster >= 2 && cluster < 0xFFF8) {
        uint16_t next = fat_table_[cluster];
        fat_table_[cluster] = 0x0000;
        cluster = next;
    }
}

void Fat16::flush_fat() {
    uint32_t fat_sectors = bpb_.fat_size_16;
    
    for (uint32_t s = 0; s < fat_sectors; s++) {
        uint16_t* entries = (uint16_t*)dma_buffer_;
        for (int i = 0; i < 256; i++) {
            uint32_t idx = s * 256 + i;
            entries[i] = (idx < FAT16_MAX_FAT_ENTRIES) ? fat_table_[idx] : 0;
        }
        
        for (uint8_t f = 0; f < bpb_.num_fats; f++) {
            Disk::write_sectors(fat_start_lba_ + f * bpb_.fat_size_16 + s, 1, dma_buffer_);
        }
    }
    cached_sector_ = 0xFFFFFFFF;
}

bool Fat16::write_file(const char* name, const uint8_t* data, uint32_t size) {
    if (!mounted_) return false;
    
    uint32_t old_sector;
    int old_index;
    if (find_dir_entry(name, &old_sector, &old_index) == 0) {
        Disk::read_sectors(old_sector, 1, dma_buffer_);
        FAT16_DirEntry* entries = (FAT16_DirEntry*)dma_buffer_;
        if (entries[old_index].first_cluster >= 2) {
            free_chain(entries[old_index].first_cluster);
        }
        entries[old_index].name[0] = 0xE5;
        Disk::write_sectors(old_sector, 1, dma_buffer_);
    }
    
    uint16_t first_cluster = 0;
    uint16_t prev_cluster = 0;
    uint32_t bytes_written = 0;
    
    while (bytes_written < size) {
        uint16_t cluster = alloc_cluster();
        if (cluster == 0) {
            if (first_cluster) free_chain(first_cluster);
            printf("[FAT16] Disk full!\n");
            flush_fat();
            return false;
        }
        
        if (first_cluster == 0) first_cluster = cluster;
        if (prev_cluster != 0) fat_table_[prev_cluster] = cluster;
        
        uint32_t lba = cluster_to_lba(cluster);
        
        for (uint8_t s = 0; s < bpb_.sectors_per_cluster && bytes_written < size; s++) {
            for (int b = 0; b < 512; b++) dma_buffer_[b] = 0;
            
            uint32_t to_copy = 512;
            if (bytes_written + to_copy > size) to_copy = size - bytes_written;
            
            for (uint32_t b = 0; b < to_copy; b++) {
                dma_buffer_[b] = data[bytes_written + b];
            }
            
            Disk::write_sectors(lba + s, 1, dma_buffer_);
            bytes_written += to_copy;
        }
        
        prev_cluster = cluster;
    }
    
    flush_fat();
    
    for (uint32_t s = 0; s < root_dir_sectors_; s++) {
        if (!Disk::read_sectors(root_dir_lba_ + s, 1, dma_buffer_)) continue;
        
        FAT16_DirEntry* entries = (FAT16_DirEntry*)dma_buffer_;
        int entries_per_sector = 512 / sizeof(FAT16_DirEntry);
        
        for (int i = 0; i < entries_per_sector; i++) {
            if (entries[i].name[0] == 0x00 || (uint8_t)entries[i].name[0] == 0xE5) {
                format_83_name(name, entries[i].name);
                entries[i].attributes = FAT_ATTR_ARCHIVE;
                for (int r = 0; r < 10; r++) entries[i].reserved[r] = 0;
                entries[i].time = RTC::fat_time();
                entries[i].date = RTC::fat_date();
                entries[i].first_cluster = first_cluster;
                entries[i].file_size = size;
                
                Disk::write_sectors(root_dir_lba_ + s, 1, dma_buffer_);
                return true;
            }
        }
    }
    
    printf("[FAT16] Root directory full!\n");
    return false;
}

bool Fat16::delete_file(const char* name) {
    if (!mounted_) return false;
    
    uint32_t sector;
    int index;
    if (find_dir_entry(name, &sector, &index) != 0) {
        printf("File not found: %s\n", name);
        return false;
    }
    
    Disk::read_sectors(sector, 1, dma_buffer_);
    FAT16_DirEntry* entries = (FAT16_DirEntry*)dma_buffer_;
    
    if (entries[index].first_cluster >= 2) {
        free_chain(entries[index].first_cluster);
        flush_fat();
    }
    
    entries[index].name[0] = 0xE5;
    Disk::write_sectors(sector, 1, dma_buffer_);
    
    return true;
}

void Fat16::stat_file(const char* name) {
    if (!mounted_) {
        printf("No filesystem mounted\n");
        return;
    }
    
    uint32_t sector;
    int index;
    if (find_dir_entry(name, &sector, &index) != 0) {
        printf("File not found: %s\n", name);
        return;
    }
    
    Disk::read_sectors(sector, 1, dma_buffer_);
    FAT16_DirEntry* entry = &((FAT16_DirEntry*)dma_buffer_)[index];
    
    char fname[13];
    int ci = 0;
    for (int j = 0; j < 8 && entry->name[j] != ' '; j++)
        fname[ci++] = entry->name[j];
    if (entry->ext[0] != ' ') {
        fname[ci++] = '.';
        for (int j = 0; j < 3 && entry->ext[j] != ' '; j++)
            fname[ci++] = entry->ext[j];
    }
    fname[ci] = '\0';
    
    printf("\n  File: %s\n", fname);
    printf("  Size: %d bytes\n", entry->file_size);
    printf("  Cluster: %d (LBA %d)\n", entry->first_cluster, cluster_to_lba(entry->first_cluster));
    printf("  Attr: ");
    if (entry->attributes & FAT_ATTR_READONLY) printf("R ");
    if (entry->attributes & FAT_ATTR_HIDDEN) printf("H ");
    if (entry->attributes & FAT_ATTR_SYSTEM) printf("S ");
    if (entry->attributes & FAT_ATTR_DIRECTORY) printf("D ");
    if (entry->attributes & FAT_ATTR_ARCHIVE) printf("A ");
    printf("\n");
    
    uint16_t t = entry->time;
    uint16_t d = entry->date;
    printf("  Modified: %d-%d-%d %d:%d:%d\n",
        1980 + (d >> 9), (d >> 5) & 0xF, d & 0x1F,
        t >> 11, (t >> 5) & 0x3F, (t & 0x1F) * 2);
    
    uint16_t cluster = entry->first_cluster;
    int chain_len = 0;
    while (cluster >= 2 && cluster < 0xFFF8) {
        chain_len++;
        cluster = fat_table_[cluster];
    }
    printf("  Clusters: %d (%d bytes)\n\n", chain_len, chain_len * bpb_.sectors_per_cluster * 512);
}

int Fat16::fat16_read(vnode* vn, uint32_t offset, uint8_t* buffer, uint32_t size) {
    if (!mounted_) return -1;
    if (vn->type != VnodeType::File) return -1;

    FAT16_DirEntry entry;
    uint32_t sector;
    int index;
    
    // Simplification: In a full VFS, vn->fs_data would store the starting cluster.
    // Here we use lookup by name (a hack for this MVP transition)
    // We assume vn->fs_data stores the name temporarily, or we just rely on fat_root lookup
    
    // Since we don't have proper path resolution passing name to vnode,
    // we use read_file_offset directly for now, which takes a name. 
    // THIS IS A TEMPORARY VFS HACK.
    const char* filename = (const char*)vn->fs_data;
    if (!filename) return -1;
    
    return read_file_offset(filename, offset, buffer, size);
}

int Fat16::fat16_write(vnode* vn, uint32_t offset, const uint8_t* buffer, uint32_t size) {
    (void)vn; (void)offset; (void)buffer; (void)size;
    // Not fully implemented for VFS in this MVP, requires extending FAT16 write_file to support offsets
    return -1;
}

int Fat16::fat16_open(vnode* vn) {
    // Only file opening supported in this minimal VFS translation
    if (!vn || vn->type != VnodeType::File) return -1;
    return 0; // Success
}

int Fat16::fat16_close(vnode* vn) {
    (void)vn;
    return 0;
}

int Fat16::fat16_lookup(vnode* dir, const char* name, vnode** out) {
    (void)dir;
    if (!mounted_) return -1;
    
    uint32_t sector;
    int index;
    if (find_dir_entry(name, &sector, &index) != 0) return -1;

    // Read the entry to get details
    Disk::read_sectors(sector, 1, dma_buffer_);
    FAT16_DirEntry* entry = &((FAT16_DirEntry*)dma_buffer_)[index];

    vnode* vn = (vnode*)kmalloc(sizeof(vnode));
    if (!vn) return -1;

    vn->type = (entry->attributes & FAT_ATTR_DIRECTORY) ? VnodeType::Directory : VnodeType::File;
    vn->size = entry->file_size;
    vn->inode_num = entry->first_cluster;
    vn->refcount = 1;
    vn->sb = nullptr; // filled by mount
    vn->ops = &fat16_vnode_ops;
    vn->mount_target = nullptr;
    
    // Store name dynamically for read hack
    int nlen = 0;
    while (name[nlen]) nlen++;
    char* name_copy = (char*)kmalloc(nlen + 1);
    for (int i = 0; i < nlen; i++) name_copy[i] = name[i];
    name_copy[nlen] = '\0';
    vn->fs_data = name_copy;

    *out = vn;
    return 0;
}

int Fat16::fat16_create(vnode* dir, const char* name, int mode, vnode** out) {
    (void)dir; (void)mode;
    
    // Very simplified create wrapper over FAT16 write_file
    uint8_t dummy = 0;
    if (!write_file(name, &dummy, 0)) return -1;
    
    return fat16_lookup(dir, name, out);
}

int Fat16::fat16_mount(block_device* bdev, superblock* sb) {
    (void)bdev;
    if (!init()) return -1; // Fallback to our hardcoded ATA/Disk init for now

    vnode* root = (vnode*)kmalloc(sizeof(vnode));
    if (!root) return -1;

    root->type = VnodeType::Directory;
    root->size = 0;
    root->inode_num = 0; // Root has no specific cluster in FAT16
    root->refcount = 1;
    root->sb = sb;
    root->ops = &fat16_vnode_ops;
    root->mount_target = nullptr;
    root->fs_data = nullptr;

    sb->root_vnode = root;
    return 0;
}

vnode_operations Fat16::fat16_vnode_ops = {
    Fat16::fat16_open,
    Fat16::fat16_close,
    Fat16::fat16_read,
    Fat16::fat16_write,
    Fat16::fat16_lookup,
    Fat16::fat16_create,
    nullptr, // mkdir
    nullptr  // unlink
};

vfs_filesystem_driver fat16_driver = {
    "fat16",
    Fat16::fat16_mount,
    nullptr // unmount
};

} // namespace re36
