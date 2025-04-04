/**
 * @file time.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * System time structure
 */

#ifndef TIME_H_
#define TIME_H_

#include <stddef.h>

typedef struct {
    int sec;    /// Seconds [0,59]
    int min;    /// Minutes [0,59]
    int hour;   /// Hour [0,23]
    int mday;   /// Day of month [1,31]
    int mon;    /// Month of year [0,11]
    int year;   /// Years since 1900
    int wday;   /// Day of week [0,6] (Sunday = 0)
    int yday;   /// Day of year [0,365]
    int yn;     /// Year number [0,99]
    int cen;    /// Century [19,20]
    int leap;   /// If year is a leap year (True == 1)
} time_t;

typedef enum {
    TZ_UTC = 0,
    TZ_EST = -5,
    TZ_EDT = -4,
} timezone_t;

/**
 * Sets the current timezone
 */
extern void set_timezone(timezone_t tz);

/**
 * Returns current time in UTC
 */
extern time_t get_utctime(void);

/**
 * Returns current time from current Timezone
 */
extern time_t get_localtime(void);

/**
 * Return the time on the system clock
 */
extern size_t get_systemtime(void);

/**
 * Converts the time into a string format
 *
 * @param time - the current time
 * @param format - see manpage for date
 * @param buf - the buffer to store it in
 */
extern void timetostr(time_t *time, char *format, char *buf, size_t n);

#endif /* time.h */
