/**
 * @file time.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * System time structure
 */

#ifndef _TIME_H
#define _TIME_H

#include <stddef.h>
#include <stdint.h>

struct time {
	int sec; /// Seconds [0,59]
	int min; /// Minutes [0,59]
	int hour; /// Hour [0,23]
	int mday; /// Day of month [1,31]
	int mon; /// Month of year [0,11]
	int year; /// Years since 1900
	int wday; /// Day of week [0,6] (Sunday = 0)
	int yday; /// Day of year [0,365]
	int yn; /// Year number [0,99]
	int cen; /// Century [19,20]
	int leap; /// If year is a leap year (True == 1)
};

/**
 * Return the current time in the system
 */
void gettime(struct time *time);

/**
 * Return current UTC time
 */
uint64_t unixtime(void);

#endif /* time.h */
