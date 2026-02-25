#include "kernel/fat16.h"
#include "kernel/ata.h"
#include "libc.h"

namespace re36 {

FAT16_BPB Fat16::bpb_;
uint16_t Fat16::fat_table_[FAT16_MAX_FAT_ENTRIES];
uint32_t Fat16::fat_start_lba_ = 0;
uint32_t Fat16::root_dir_lba_ = 0;
uint32_t Fat16::data_start_lba_ = 0;
uint32_t Fat16::root_dir_sectors_ = 0;
bool Fat16::mounted_ = false;
uint8_t Fat16::sector_cache_[FAT16_SECTOR_BUF_SIZE];
uint32_t Fat16::cached_sector_ = 0xFFFFFFFF;

bool Fat16::read_cached_sector(uint32_t lba) {
    if (lba == cached_sector_) return true;
    if (!ATA::read_sectors(lba, 1, sector_cache_)) return false;
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
    if (!ATA::is_present()) {
        printf("[FAT16] No ATA disk found\n");
        return false;
    }

    uint8_t boot_sector[512];
    if (!ATA::read_sectors(0, 1, boot_sector)) {
        printf("[FAT16] Failed to read boot sector\n");
        return false;
    }

    FAT16_BPB* bpb = (FAT16_BPB*)boot_sector;
    
    for (uint32_t i = 0; i < sizeof(FAT16_BPB); i++) {
        ((uint8_t*)&bpb_)[i] = boot_sector[i];
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

    uint8_t fat_buf[512];
    uint32_t entry_idx = 0;
    for (uint32_t s = 0; s < fat_sectors && entry_idx < fat_entries; s++) {
        if (!ATA::read_sectors(fat_start_lba_ + s, 1, fat_buf)) {
            printf("[FAT16] Failed to read FAT sector %d\n", s);
            return false;
        }
        for (uint32_t i = 0; i < 256 && entry_idx < fat_entries; i++) {
            fat_table_[entry_idx++] = ((uint16_t*)fat_buf)[i];
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

    uint8_t dir_buf[512];
    int file_count = 0;

    for (uint32_t s = 0; s < root_dir_sectors_; s++) {
        if (!ATA::read_sectors(root_dir_lba_ + s, 1, dir_buf)) continue;

        FAT16_DirEntry* entries = (FAT16_DirEntry*)dir_buf;
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

    uint8_t dir_buf[512];
    for (uint32_t s = 0; s < root_dir_sectors_; s++) {
        if (!ATA::read_sectors(root_dir_lba_ + s, 1, dir_buf)) continue;

        FAT16_DirEntry* entries = (FAT16_DirEntry*)dir_buf;
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
    uint32_t cluster_size = bpb_.sectors_per_cluster * 512;

    while (cluster >= 2 && cluster < 0xFFF8 && bytes_read < max_size) {
        uint32_t lba = cluster_to_lba(cluster);

        for (uint8_t s = 0; s < bpb_.sectors_per_cluster; s++) {
            if (bytes_read >= max_size) break;
            if (bytes_read >= found_entry.file_size) break;

            uint8_t sec_buf[512];
            if (!ATA::read_sectors(lba + s, 1, sec_buf)) return bytes_read;

            uint32_t to_copy = 512;
            if (bytes_read + to_copy > found_entry.file_size)
                to_copy = found_entry.file_size - bytes_read;
            if (bytes_read + to_copy > max_size)
                to_copy = max_size - bytes_read;

            for (uint32_t b = 0; b < to_copy; b++) {
                buffer[bytes_read + b] = sec_buf[b];
            }
            bytes_read += to_copy;
        }

        cluster = fat_table_[cluster];
    }

    return bytes_read;
}

bool Fat16::is_mounted() {
    return mounted_;
}

} // namespace re36
