#include <time.h>
#include "../../syscalls.h"

extern "C" {

time_t time(time_t *timer) {
    time_t t = sys_time();
    if (timer) {
        *timer = t;
    }
    return t;
}

clock_t clock(void) {
    return sys_uptime();
}

static struct tm _tm_static;
static const unsigned short days_per_month[12] = {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

struct tm *localtime(const time_t *timer) {
    if (!timer) return 0;
    
    time_t t = *timer;
    
    _tm_static.tm_sec = t % 60;
    t /= 60;
    _tm_static.tm_min = t % 60;
    t /= 60;
    _tm_static.tm_hour = t % 24;
    t /= 24;
    
    // t is now days since Jan 1 1970
    // Jan 1 1970 was a Thursday (4)
    _tm_static.tm_wday = (t + 4) % 7;
    
    int year = 1970;
    while (true) {
        bool is_leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
        int days_in_year = is_leap ? 366 : 365;
        if (t >= (unsigned int)days_in_year) {
            t -= days_in_year;
            year++;
        } else {
            break;
        }
    }
    
    _tm_static.tm_year = year - 1900;
    _tm_static.tm_yday = t;
    
    bool is_leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
    int mon = 0;
    while (mon < 12) {
        int dim = days_per_month[mon];
        if (mon == 1 && is_leap) dim = 29;
        
        if (t >= (unsigned int)dim) {
            t -= dim;
            mon++;
        } else {
            break;
        }
    }
    
    _tm_static.tm_mon = mon;
    _tm_static.tm_mday = t + 1;
    _tm_static.tm_isdst = 0; // Not supported currently
    
    return &_tm_static;
}

} // extern "C"
