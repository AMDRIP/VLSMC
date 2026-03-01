#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int time_t;
typedef unsigned int clock_t;

#define CLOCKS_PER_SEC 1000

struct tm {
    int tm_sec;   // seconds after the minute [0-60]
    int tm_min;   // minutes after the hour [0-59]
    int tm_hour;  // hours since midnight [0-23]
    int tm_mday;  // day of the month [1-31]
    int tm_mon;   // months since January [0-11]
    int tm_year;  // years since 1900
    int tm_wday;  // days since Sunday [0-6]
    int tm_yday;  // days since January 1 [0-365]
    int tm_isdst; // Daylight Savings Time flag
};

time_t time(time_t *timer);
clock_t clock(void);
struct tm *localtime(const time_t *timer);

#ifdef __cplusplus
}
#endif
