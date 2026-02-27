#pragma once

#include <stdint.h>
#include "kernel/pci.h"

namespace re36 {

#define HBA_PORT_IPM_ACTIVE 1
#define HBA_PORT_DET_PRESENT 3

// Заголовок команды (32 байта)
struct HBA_CMD_HEADER {
    uint8_t  cfl:5;    // Длина FIS в dwords (обычно 5)
    uint8_t  a:1;      // ATAPI
    uint8_t  w:1;      // Направление (1 - H2D, 0 - D2H)
    uint8_t  p:1;      // Prefetchable
    uint8_t  r:1;      // Reset
    uint8_t  b:1;      // BIST
    uint8_t  c:1;      // Clear busy upon R_OK
    uint8_t  res0:1;
    uint8_t  pmp:4;    // Port multiplier port
    uint16_t prdtl;    // Количество PRDT записей
    uint32_t prdbc;    // Передано байт
    uint32_t ctba;     // Физический адрес Command Table (Low)
    uint32_t ctbau;    // Физический адрес Command Table (High)
    uint32_t res1[4];
} __attribute__((packed));

// Элемент PRDT (16 байт)
struct HBA_PRDT_ENTRY {
    uint32_t dba;      // Физический адрес буфера (Low)
    uint32_t dbau;     // Физический адрес буфера (High)
    uint32_t res0;
    uint32_t dbc:22;   // Размер данных (Count - 1)
    uint32_t res1:9;
    uint32_t i:1;      // Прерывание по завершении
} __attribute__((packed));

// Command Table
struct HBA_CMD_TBL {
    uint8_t  cfis[64]; // Command FIS
    uint8_t  acmd[16]; // ATAPI command
    uint8_t  res[48];  // Reserved
    HBA_PRDT_ENTRY prdt_entry[1]; // PRDT (We can have up to 65535, but we allocate dynamically or fix size)
} __attribute__((packed));

struct HBA_PORT {
    uint32_t clb;      // 0x00
    uint32_t clbu;     // 0x04
    uint32_t fb;       // 0x08
    uint32_t fbu;      // 0x0C
    uint32_t is;       // 0x10 Interrupt Status
    uint32_t ie;       // 0x14 Interrupt Enable
    uint32_t cmd;      // 0x18 Command and Status
    uint32_t rsv0;     // 0x1C
    uint32_t tfd;      // 0x20 Task File Data
    uint32_t sig;      // 0x24 Signature
    uint32_t ssts;     // 0x28 SATA Status 
    uint32_t sctl;     // 0x2C
    uint32_t serr;     // 0x30
    uint32_t sact;     // 0x34
    uint32_t ci;       // 0x38 Command Issue
    uint32_t sntf;     // 0x3C
    uint32_t fbs;      // 0x40
    uint32_t rsv1[11]; // 0x44 -> 0x6F
    uint32_t vendor[4];// 0x70 -> 0x7F
} __attribute__((packed));

struct HBA_MEM {
    uint32_t cap;      // 0x00 Host Capabilities
    uint32_t ghc;      // 0x04 Global Host Control
    uint32_t is;       // 0x08 Interrupt Status
    uint32_t pi;       // 0x0C Ports Implemented
    uint32_t vs;       // 0x10 Version
    uint32_t ccc_ctl;  // 0x14
    uint32_t ccc_pts;  // 0x18
    uint32_t em_loc;   // 0x1C
    uint32_t em_ctl;   // 0x20
    uint32_t cap2;     // 0x24
    uint32_t bohc;     // 0x28
    uint8_t  rsv[0x74];// 0x2C -> 0x9F
    uint8_t  vendor[0x60]; // 0xA0 -> 0xFF
    HBA_PORT ports[32]; // 0x100 -> ...
} __attribute__((packed));

// FIS типов
#define FIS_TYPE_REG_H2D 0x27

struct FIS_REG_H2D {
    uint8_t  fis_type;
    uint8_t  pmport:4;
    uint8_t  rsv0:3;
    uint8_t  c:1;      // Command = 1
    uint8_t  command;
    uint8_t  featurel;
    uint8_t  lba0;
    uint8_t  lba1;
    uint8_t  lba2;
    uint8_t  device;
    uint8_t  lba3;
    uint8_t  lba4;
    uint8_t  lba5;
    uint8_t  featureh;
    uint8_t  countl;
    uint8_t  counth;
    uint8_t  icc;
    uint8_t  control;
    uint8_t  rsv1[4];
} __attribute__((packed));

#define AHCI_DEV_SATA 1
#define AHCI_DEV_SATAPI 2
#define AHCI_DEV_SEMB 3
#define AHCI_DEV_PM 4
#define AHCI_DEV_NULL 0

class AHCIDriver {
public:
    static void init();
    
    // Read sectors from a specific port
    static bool read(uint8_t port, uint64_t lba, uint32_t sector_count, void* buffer_phys);
    
    // Write sectors to a specific port
    static bool write(uint8_t port, uint64_t lba, uint32_t sector_count, void* buffer_phys);

    static bool is_present();
    static int get_primary_port();

private:
    static PCIDevice* find_ahci_controller();
    static void check_port(HBA_PORT* port, int port_no);
    static void port_rebase(HBA_PORT* port, int port_no);
    static void start_cmd(HBA_PORT* port);
    static void stop_cmd(HBA_PORT* port);
    static int find_cmdslot(HBA_PORT* port);

    static HBA_MEM* abarz;
};

} // namespace re36
