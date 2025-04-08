#include <lib.h>
#include <comus/asm.h>
#include <comus/time.h>
#include <comus/drivers/clock.h>

#define CMOS_WRITE_PORT 0x70
#define CMOS_READ_PORT 0x71

#define CMOS_REG_SEC 0x00
#define CMOS_REG_MIN 0x02
#define CMOS_REG_HOUR 0x04
#define CMOS_REG_WDAY 0x06
#define CMOS_REG_MDAY 0x07
#define CMOS_REG_MON 0x08
#define CMOS_REG_YEAR 0x09
#define CMOS_REG_CEN 0x32

static uint8_t cmos_read(uint8_t reg)
{
	uint8_t hex, ret;

	outb(CMOS_WRITE_PORT, reg);
	hex = inb(CMOS_READ_PORT);

	ret = hex & 0x0F;
	ret += (hex & 0xF0) / 16 * 10;

	return ret;
}

static int mday_offset[12] = { 0,	31,	 59,  90,  120, 151,
							   181, 212, 243, 273, 304, 334 };

void gettime(struct time *time)
{
	time->sec = cmos_read(CMOS_REG_SEC);
	time->min = cmos_read(CMOS_REG_MIN);
	time->hour = cmos_read(CMOS_REG_HOUR);
	time->wday = cmos_read(CMOS_REG_WDAY) - 1;
	time->mday = cmos_read(CMOS_REG_MDAY);
	time->mon = cmos_read(CMOS_REG_MON) - 1;
	time->yn = cmos_read(CMOS_REG_YEAR);
	time->cen = 20;

	time->year = time->yn + time->cen * 100;

	time->leap = time->year % 4 == 0 && time->year % 100 != 0;

	time->yday = mday_offset[time->mon] + time->mday;

	if (time->leap && time->mon > 2)
		time->yday++;

	time->year -= 1900;
}

uint64_t unixtime(void)
{
	struct time time;
	gettime(&time);

	// FIXME: probably wrong
	uint64_t unix = 0;
	unix += time.sec;
	unix += time.min * 60;
	unix += time.hour * 60 * 60;
	unix += time.yday * 60 * 60 * 24;
	unix += time.year * 60 * 60 * 24 * 365;
	return unix;
}
