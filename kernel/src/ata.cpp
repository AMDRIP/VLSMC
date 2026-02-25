#include "kernel/ata.h"
#include "kernel/pic.h"
#include "libc.h"

namespace re36 {

bool ATA::present_ = false;

void ATA::wait_bsy() {
    while (inb(ATA_PRIMARY_IO + ATA_REG_STATUS) & ATA_SR_BSY);
}

void ATA::wait_drq() {
    while (!(inb(ATA_PRIMARY_IO + ATA_REG_STATUS) & ATA_SR_DRQ));
}

bool ATA::init() {
    outb(ATA_PRIMARY_IO + ATA_REG_DRIVE, 0xA0);

    outb(ATA_PRIMARY_IO + ATA_REG_SECCOUNT, 0);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_LO, 0);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_MID, 0);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_HI, 0);

    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

    uint8_t status = inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
    if (status == 0) {
        present_ = false;
        return false;
    }

    wait_bsy();

    if (inb(ATA_PRIMARY_IO + ATA_REG_LBA_MID) != 0 ||
        inb(ATA_PRIMARY_IO + ATA_REG_LBA_HI) != 0) {
        present_ = false;
        return false;
    }

    while (true) {
        status = inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
        if (status & ATA_SR_ERR) {
            present_ = false;
            return false;
        }
        if (status & ATA_SR_DRQ) break;
    }

    uint16_t identify_data[256];
    for (int i = 0; i < 256; i++) {
        identify_data[i] = inw(ATA_PRIMARY_IO + ATA_REG_DATA);
    }

    uint32_t total_sectors = identify_data[60] | ((uint32_t)identify_data[61] << 16);
    (void)total_sectors;

    present_ = true;
    return true;
}

bool ATA::read_sectors(uint32_t lba, uint8_t count, void* buffer) {
    if (!present_) return false;

    wait_bsy();

    outb(ATA_PRIMARY_IO + ATA_REG_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_IO + ATA_REG_SECCOUNT, count);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_LO, (uint8_t)(lba & 0xFF));
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_HI, (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

    uint16_t* buf = (uint16_t*)buffer;

    for (int s = 0; s < count; s++) {
        wait_bsy();

        uint8_t status = inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
        if (status & ATA_SR_ERR) return false;

        wait_drq();

        for (int i = 0; i < 256; i++) {
            buf[s * 256 + i] = inw(ATA_PRIMARY_IO + ATA_REG_DATA);
        }
    }

    return true;
}

bool ATA::is_present() {
    return present_;
}

bool ATA::write_sectors(uint32_t lba, uint8_t count, const void* buffer) {
    if (!present_) return false;

    wait_bsy();

    outb(ATA_PRIMARY_IO + ATA_REG_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_IO + ATA_REG_SECCOUNT, count);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_LO, (uint8_t)(lba & 0xFF));
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_HI, (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);

    const uint16_t* buf = (const uint16_t*)buffer;

    for (int s = 0; s < count; s++) {
        wait_bsy();
        wait_drq();

        for (int i = 0; i < 256; i++) {
            outw(ATA_PRIMARY_IO + ATA_REG_DATA, buf[s * 256 + i]);
        }
    }

    wait_bsy();
    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, 0xE7);
    wait_bsy();

    return true;
}

} // namespace re36
