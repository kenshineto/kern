#include <lib.h>
#include <time.h>
#include <comus/asm.h>
#include <comus/drivers/clock.h>

#define CMOS_WRITE_PORT 0x70
#define CMOS_READ_PORT  0x71

#define CMOS_REG_SEC    0x00
#define CMOS_REG_MIN    0x02
#define CMOS_REG_HOUR   0x04
#define CMOS_REG_WDAY   0x06
#define CMOS_REG_MDAY   0x07
#define CMOS_REG_MON    0x08
#define CMOS_REG_YEAR   0x09
#define CMOS_REG_CEN    0x32

// Live buffers to work on data
static time_t time;
static time_t localtime;

// Front buffers so interupts dont request data that is half done
static time_t curr_time;
static time_t curr_localtime;

// Current set time Zone
static timezone_t curr_timezone = TZ_UTC;
static timezone_t last_timezone = TZ_UTC;

static uint8_t cmos_read(uint8_t reg) {
    uint8_t hex, ret;

    outb(CMOS_WRITE_PORT, reg);
    hex = inb(CMOS_READ_PORT);

    ret = hex & 0x0F;
    ret += (hex & 0xF0) / 16 * 10;

    return ret;
}

static int mday_offset[12] = {
    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};

static int month_days[12] = {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static void update_localtime(void) {

    int change, max;

    // set localtime
    localtime = time;

    // if tz is UTC, we dont need to do anythin
    if (last_timezone == TZ_UTC) return;

    localtime.hour += last_timezone;

    // check if day rolled over
    change = localtime.hour < 0 ? -1 : localtime.hour >= 24 ? 1 : 0;
    if (!change) return;

    // roll over day
    localtime.hour = (localtime.hour + 24) % 24;
    localtime.wday = (localtime.wday + change + 7) % 7;
    localtime.mday += change;
    localtime.yday += change;

    // check if month rolled over
    max = month_days[localtime.mon];
    if (localtime.leap && localtime.mon == 1) max++;
    change = localtime.mday < 0 ? -1 : localtime.mday >= max ? 1 : 0;
    if (!change) return;

    // roll over month
    localtime.mon = (localtime.mon + change + 12) % 12;

    // check if year rolled over
    max = localtime.leap ? 366 : 365;
    change = localtime.yday < 0 ? -1 : localtime.yday >= max ? 1 : 0;
    if (!change) return;

    // roll over year
    localtime.yn += change;

    // check if cen rolled over
    change = localtime.yn < 0 ? -1 : localtime.yn >= 100 ? 1 : 0;
    if (!change) goto year;

    // roll over cen
    localtime.cen += change;


year:

    localtime.year = localtime.yn + localtime.cen * 100;
    localtime.leap = localtime.year % 4 == 0 && localtime.year % 100 != 0;

    if (localtime.leap && localtime.yday == -1)
        localtime.yday = 365;
    else if (localtime.yday == -1)
        localtime.yday = 364;
    else
        localtime.yday = 0;

    localtime.year -= 1900;

}

void clock_update(void) {
    time.sec = cmos_read(CMOS_REG_SEC);
    time.min = cmos_read(CMOS_REG_MIN);
    time.hour = cmos_read(CMOS_REG_HOUR);
    time.wday = cmos_read(CMOS_REG_WDAY) - 1;
    time.mday = cmos_read(CMOS_REG_MDAY);
    time.mon = cmos_read(CMOS_REG_MON) - 1;
    time.yn = cmos_read(CMOS_REG_YEAR);
    time.cen = 20;

    time.year = time.yn + time.cen * 100;

    time.leap = time.year % 4 == 0 && time.year % 100 != 0;

    time.yday = mday_offset[time.mon] + time.mday;

    if (time.leap && time.mon > 2)
        time.yday++;

    time.year -= 1900;

    update_localtime();

    curr_time = time;
    curr_localtime = localtime;
}

void set_timezone(timezone_t tz) {
    curr_timezone = tz;
}

time_t get_utctime(void) {
    return curr_time;
}

time_t get_localtime(void) {
    if (curr_timezone != last_timezone) {
        last_timezone = curr_timezone;
        update_localtime();
        curr_localtime = localtime;
    }
    return curr_localtime;
}

size_t get_systemtime(void) {
    return 0;
}
