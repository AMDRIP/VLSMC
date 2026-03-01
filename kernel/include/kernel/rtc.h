#pragma once

#include <stdint.h>

namespace re36 {

struct DateTime {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;
    uint8_t month;
    uint16_t year;
};

class RTC {
public:
    static void init(bool enable_irq = false);
    static void read(DateTime& dt);
    static uint16_t fat_time();
    static uint16_t fat_date();
    static uint32_t uptime_seconds();
    static uint32_t to_unix_timestamp();
    static void on_irq();

private:
    static uint8_t read_register(uint8_t reg);
    static void write_register(uint8_t reg, uint8_t val);
    static bool is_updating();
    static uint8_t bcd_to_bin(uint8_t bcd);
    static uint8_t detect_century_register();

    static volatile uint32_t irq_ticks_;
    static bool irq_enabled_;
    static uint8_t century_reg_;
};

} // namespace re36
