#pragma once

#include <stdint.h>
#include <stddef.h>

namespace re36 {

#define ATA_PRIMARY_IO    0x1F0
#define ATA_PRIMARY_CTRL  0x3F6

#define ATA_REG_DATA      0
#define ATA_REG_ERROR     1
#define ATA_REG_SECCOUNT  2
#define ATA_REG_LBA_LO    3
#define ATA_REG_LBA_MID   4
#define ATA_REG_LBA_HI    5
#define ATA_REG_DRIVE     6
#define ATA_REG_STATUS    7
#define ATA_REG_COMMAND   7

#define ATA_CMD_READ_PIO  0x20
#define ATA_CMD_IDENTIFY  0xEC

#define ATA_SR_BSY        0x80
#define ATA_SR_DRDY       0x40
#define ATA_SR_DRQ        0x08
#define ATA_SR_ERR        0x01

#define ATA_SECTOR_SIZE   512

class ATA {
public:
    static bool init();
    
    static bool read_sectors(uint32_t lba, uint8_t count, void* buffer);
    
    static bool is_present();

private:
    static void wait_bsy();
    static void wait_drq();
    static bool present_;
};

} // namespace re36
