#pragma once

#include <stdint.h>
#include <stddef.h>

namespace re36 {

struct __attribute__((packed)) FAT16_BPB {
    uint8_t  jmp[3];
    char     oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  num_fats;
    uint16_t root_entry_count;
    uint16_t total_sectors_16;
    uint8_t  media_type;
    uint16_t fat_size_16;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
};

struct __attribute__((packed)) FAT16_DirEntry {
    char     name[8];
    char     ext[3];
    uint8_t  attributes;
    uint8_t  reserved[10];
    uint16_t time;
    uint16_t date;
    uint16_t first_cluster;
    uint32_t file_size;
};

#define FAT_ATTR_READONLY  0x01
#define FAT_ATTR_HIDDEN    0x02
#define FAT_ATTR_SYSTEM    0x04
#define FAT_ATTR_VOLUME_ID 0x08
#define FAT_ATTR_DIRECTORY 0x10
#define FAT_ATTR_ARCHIVE   0x20
#define FAT_ATTR_LFN       0x0F

#define FAT16_MAX_FAT_ENTRIES 32768
#define FAT16_SECTOR_BUF_SIZE 512

class Fat16 {
public:
    static bool init();
    
    static void list_root();
    
    static int read_file(const char* name, uint8_t* buffer, uint32_t max_size);
    
    static int read_file_offset(const char* name, uint32_t offset, uint8_t* buffer, uint32_t size);
    
    static bool write_file(const char* name, const uint8_t* data, uint32_t size);
    
    static bool delete_file(const char* name);
    
    static void stat_file(const char* name);
    
    static bool is_mounted();
    
    static uint32_t root_dir_lba() { return root_dir_lba_; }

private:
    static FAT16_BPB bpb_;
    static uint16_t fat_table_[FAT16_MAX_FAT_ENTRIES];
    static uint32_t fat_start_lba_;
    static uint32_t root_dir_lba_;
    static uint32_t data_start_lba_;
    static uint32_t root_dir_sectors_;
    static bool mounted_;
    
    static uint8_t sector_cache_[FAT16_SECTOR_BUF_SIZE];
    static uint32_t cached_sector_;
    
    static bool read_cached_sector(uint32_t lba);
    static uint32_t cluster_to_lba(uint16_t cluster);
    static bool match_filename(const FAT16_DirEntry* entry, const char* name);
    static uint16_t alloc_cluster();
    static void free_chain(uint16_t start_cluster);
    static void flush_fat();
    static void format_83_name(const char* name, char* out);
    static int find_dir_entry(const char* name, uint32_t* sector_out, int* index_out);
};

} // namespace re36
