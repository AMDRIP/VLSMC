#include "kernel/rtc.h"
#include "kernel/pic.h"
#include "kernel/idt.h"
#include "libc.h"

namespace re36 {

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

volatile uint32_t RTC::irq_ticks_ = 0;
bool RTC::irq_enabled_ = false;
uint8_t RTC::century_reg_ = 0;

uint8_t RTC::read_register(uint8_t reg) {
    outb(CMOS_ADDR, reg | 0x80);
    io_wait();
    return inb(CMOS_DATA);
}

void RTC::write_register(uint8_t reg, uint8_t val) {
    outb(CMOS_ADDR, reg | 0x80);
    io_wait();
    outb(CMOS_DATA, val);
}

bool RTC::is_updating() {
    outb(CMOS_ADDR, 0x0A | 0x80);
    io_wait();
    return inb(CMOS_DATA) & 0x80;
}

uint8_t RTC::bcd_to_bin(uint8_t bcd) {
    return (bcd & 0x0F) + ((bcd >> 4) * 10);
}

uint8_t RTC::detect_century_register() {
    uint8_t val = read_register(0x32);
    if (val >= 0x19 && val <= 0x21) return 0x32;
    val = bcd_to_bin(val);
    if (val >= 19 && val <= 21) return 0x32;

    val = read_register(0x37);
    if (val >= 0x19 && val <= 0x21) return 0x37;
    val = bcd_to_bin(val);
    if (val >= 19 && val <= 21) return 0x37;

    return 0;
}

void RTC::init(bool enable_irq) {
    century_reg_ = detect_century_register();

    if (enable_irq) {
        asm volatile("cli");

        uint8_t regB = read_register(0x0B);
        write_register(0x0B, regB | 0x40);

        uint8_t regA = read_register(0x0A);
        write_register(0x0A, (regA & 0xF0) | 0x0F);

        read_register(0x0C);

        outb(0xA1, inb(0xA1) & ~(1 << 0));

        irq_enabled_ = true;
        asm volatile("sti");
    }

    outb(CMOS_ADDR, 0x0D);
    inb(CMOS_DATA);
}

void RTC::on_irq() {
    irq_ticks_++;
    read_register(0x0C);
    pic_send_eoi(8);
}

void RTC::read(DateTime& dt) {
    uint32_t flags;
    asm volatile("pushf; pop %0; cli" : "=r"(flags));

    uint8_t sec, min, hour, day, mon, year, century = 0;
    uint8_t last_sec, last_min, last_hour, last_day, last_mon, last_year, last_century = 0;

    while (is_updating());

    sec  = read_register(0x00);
    min  = read_register(0x02);
    hour = read_register(0x04);
    day  = read_register(0x07);
    mon  = read_register(0x08);
    year = read_register(0x09);
    if (century_reg_) century = read_register(century_reg_);

    do {
        last_sec     = sec;
        last_min     = min;
        last_hour    = hour;
        last_day     = day;
        last_mon     = mon;
        last_year    = year;
        last_century = century;

        while (is_updating());

        sec  = read_register(0x00);
        min  = read_register(0x02);
        hour = read_register(0x04);
        day  = read_register(0x07);
        mon  = read_register(0x08);
        year = read_register(0x09);
        if (century_reg_) century = read_register(century_reg_);
    } while (sec != last_sec || min != last_min || hour != last_hour ||
             day != last_day || mon != last_mon || year != last_year ||
             century != last_century);

    uint8_t regB = read_register(0x0B);

    outb(CMOS_ADDR, 0x0D);
    inb(CMOS_DATA);

    asm volatile("push %0; popf" :: "r"(flags));

    bool is_bcd = !(regB & 0x04);
    bool is_12h = !(regB & 0x02);
    bool is_pm  = hour & 0x80;

    if (is_bcd) {
        sec  = bcd_to_bin(sec);
        min  = bcd_to_bin(min);
        hour = bcd_to_bin(hour & 0x7F);
        day  = bcd_to_bin(day);
        mon  = bcd_to_bin(mon);
        year = bcd_to_bin(year);
        if (century_reg_) century = bcd_to_bin(century);
    } else {
        hour = hour & 0x7F;
    }

    if (is_12h) {
        if (is_pm && hour != 12) {
            hour += 12;
        } else if (!is_pm && hour == 12) {
            hour = 0;
        }
    }

    dt.seconds = sec;
    dt.minutes = min;
    dt.hours   = hour;
    dt.day     = day;
    dt.month   = mon;

    if (century_reg_ && century >= 19 && century <= 21) {
        dt.year = (uint16_t)century * 100 + year;
    } else {
        dt.year = (year >= 70) ? (1900 + year) : (2000 + year);
    }
}

uint32_t RTC::uptime_seconds() {
    return irq_ticks_ / 2;
}

uint16_t RTC::fat_time() {
    DateTime dt;
    read(dt);
    return ((uint16_t)dt.hours << 11) |
           ((uint16_t)dt.minutes << 5) |
           ((uint16_t)(dt.seconds / 2));
}

uint16_t RTC::fat_date() {
    DateTime dt;
    read(dt);
    uint16_t fat_year = (dt.year >= 1980) ? (dt.year - 1980) : 0;
    return (fat_year << 9) |
           ((uint16_t)dt.month << 5) |
           ((uint16_t)dt.day);
}

} // namespace re36
