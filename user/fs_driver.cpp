#include "app_api.h"

// Простой парсер FAT16, работающий через App::read_sector
struct BootSector {
    uint8_t jump[3];
    char oem[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_count;
    uint16_t root_dir_entries;
    uint16_t total_sectors_short;
    uint8_t media_descriptor;
    uint16_t fat_size_sectors;
    uint16_t sectors_per_track;
    uint16_t heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_long;
} __attribute__((packed));

struct DirEntry {
    char name[11];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t ctime_tenths;
    uint16_t ctime;
    uint16_t cdate;
    uint16_t adate;
    uint16_t cluster_high;
    uint16_t mtime;
    uint16_t mdate;
    uint16_t cluster_low;
    uint32_t size;
} __attribute__((packed));

#define SECTOR_SIZE 512

static BootSector bs;
static uint32_t fat_start_lba;
static uint32_t root_dir_start_lba;
static uint32_t data_start_lba;

// Вспомогательные функции сравнения строк (так как libc нет)
static bool str_eq_n(const char* s1, const char* s2, int n) {
    for (int i = 0; i < n; i++) {
        if (s1[i] != s2[i]) return false;
    }
    return true;
}

static void read_fat_info() {
    uint8_t sector[SECTOR_SIZE];
    if (!vlsmc::App::read_sector(0, sector)) {
        vlsmc::App::print("[FS] Failed to read Boot Sector\n");
        return;
    }

    // Копирование Boot Sector
    for (int i = 0; i < sizeof(BootSector); i++) {
        ((uint8_t*)&bs)[i] = sector[i];
    }

    fat_start_lba = bs.reserved_sectors;
    root_dir_start_lba = fat_start_lba + (bs.fat_count * bs.fat_size_sectors);
    
    uint32_t root_dir_sectors = (bs.root_dir_entries * 32 + SECTOR_SIZE - 1) / SECTOR_SIZE;
    data_start_lba = root_dir_start_lba + root_dir_sectors;

    vlsmc::App::print("[FS] FAT16 Mounted in Ring 3!\n");
}

static uint32_t find_file(const char* filename83) {
    uint8_t sector[SECTOR_SIZE];
    uint32_t root_dir_sectors = (bs.root_dir_entries * 32 + SECTOR_SIZE - 1) / SECTOR_SIZE;

    for (uint32_t i = 0; i < root_dir_sectors; i++) {
        if (!vlsmc::App::read_sector(root_dir_start_lba + i, sector)) return 0;

        DirEntry* entries = (DirEntry*)sector;
        for (int j = 0; j < SECTOR_SIZE / 32; j++) {
            if (entries[j].name[0] == 0x00) return 0; // Конец каталога
            if (entries[j].name[0] == (char)0xE5) continue; // Удаленный файл
            if (entries[j].attributes & 0x0F) continue; // LFN или спец-атрибуты

            if (str_eq_n(entries[j].name, filename83, 11)) {
                return entries[j].cluster_low | ((uint32_t)entries[j].cluster_high << 16);
            }
        }
    }
    return 0;
}

static uint32_t get_next_cluster(uint32_t cluster) {
    uint8_t sector[SECTOR_SIZE];
    uint32_t fat_offset = cluster * 2;
    uint32_t fat_sector = fat_start_lba + (fat_offset / SECTOR_SIZE);
    uint32_t ent_offset = fat_offset % SECTOR_SIZE;

    if (!vlsmc::App::read_sector(fat_sector, sector)) return 0xFFFF;
    
    uint16_t next_cluster = *((uint16_t*)&sector[ent_offset]);
    return next_cluster;
}

static int read_file_content(uint32_t cluster, uint8_t* out_buffer, int max_size) {
    int bytes_read = 0;
    uint8_t sector[SECTOR_SIZE];

    while (cluster >= 2 && cluster <= 0xFFEF && bytes_read < max_size) {
        uint32_t lba = data_start_lba + (cluster - 2) * bs.sectors_per_cluster;
        
        for (int i = 0; i < bs.sectors_per_cluster; i++) {
            if (bytes_read >= max_size) break;
            if (!vlsmc::App::read_sector(lba + i, sector)) return bytes_read;
            
            for (int b = 0; b < SECTOR_SIZE && bytes_read < max_size; b++) {
                out_buffer[bytes_read++] = sector[b];
            }
        }
        
        cluster = get_next_cluster(cluster);
    }
    return bytes_read;
}

// Формат IPC сообщения:
// Если первый байт '-', это просто текстовый лог
// Если нужно считать файл, присылается имя "HELLO   TXT". FS отвечает содержимым.
void fs_main() {
    vlsmc::App::print("[FS] Starting Ring 3 Filesystem Driver...\n");
    read_fat_info();

    uint8_t msg_buf[512];
    int sender_tid;

    while (true) {
        int sz = vlsmc::App::msg_recv(&sender_tid, msg_buf, sizeof(msg_buf));
        if (sz > 0) {
            // Обеспечим null-терминацию для безопасного парсинга
            if (sz < 512) msg_buf[sz] = '\0';
            else msg_buf[511] = '\0';

            char filename83[11];
            bool valid_req = true;
            for(int i=0; i<11; i++) {
                if (i < sz) filename83[i] = msg_buf[i];
                else filename83[i] = ' ';
            }

            uint32_t cluster = find_file(filename83);
            if (cluster) {
                // Читаем контент файла прямо в буфер и отсылаем назад
                int read_sz = read_file_content(cluster, msg_buf, 512);
                vlsmc::App::msg_send(sender_tid, msg_buf, read_sz);
            } else {
                // Файл не найден
                const char* err = "FILE NOT FOUND";
                vlsmc::App::msg_send(sender_tid, err, 14);
            }
        } else {
            vlsmc::App::sleep(10); // Ждем 10мс вместо yield, чтобы квант прошел корректно
        }
    }
}

int main() {
    fs_main();
    return 0;
}
