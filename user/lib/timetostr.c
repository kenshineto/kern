#include <lib.h>
#include <time.h>

static char *ABB_WEEKDAY[7] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static char *FULL_WEEKDAY[7] = { "Sunday",	 "Monday", "Tuesday", "Wednesday",
								 "Thursday", "Friday", "Saturady" };

static char *ABB_MONTH[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
							   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

static char *FULL_MONTH[12] = {
	"January", "Feburary", "March",		"April",   "May",	   "June",
	"July",	   "August",   "September", "October", "November", "December"
};

static char *write_num(unsigned int num, unsigned int pad, char *buf, size_t n)
{
	size_t digits = 1;
	unsigned int x = num;

	while (x /= 10, x > 0)
		digits++;
	if (pad == 0)
		pad = digits;

	for (size_t i = 0; i < pad; i++) {
		size_t digit;
		if (i >= digits) {
			digit = 0;
		} else {
			digit = num % 10;
			num /= 10;
		}

		if (pad - i - 1 >= n)
			continue;
		buf[pad - i - 1] = '0' + digit;
	}

	if (pad > n)
		pad = n;

	return buf + pad;
}

void timetostr(time_t *time, char *format, char *buf, size_t n)
{
	char *index = buf;
	char c;
	int space;

	while (c = *format++, space = (buf + n) - index, c != '\0' && space > 0) {
		if (c != '%') {
			*index++ = c;
			continue;
		} else {
			c = *format++;
		}

		switch (c) {
		case '%':
			*index++ = '%';
			break;
		case 'a':
			index = strncpy(index, ABB_WEEKDAY[time->wday], space);
			break;
		case 'A':
			index = strncpy(index, FULL_WEEKDAY[time->wday], space);
			break;
		case 'b':
		case 'h':
			index = strncpy(index, ABB_MONTH[time->mon], space);
			break;
		case 'B':
			index = strncpy(index, FULL_MONTH[time->mon], space);
			break;
		case 'C':
			index = write_num(time->cen, 0, index, space);
			break;
		case 'd':
			index = write_num(time->mday, 2, index, space);
			break;
		case 'H':
			index = write_num(time->hour, 2, index, space);
			break;
		case 'I':
			index = write_num((time->hour + 12) % 12 + 1, 2, index, space);
			break;
		case 'j':
			index = write_num(time->yday, 3, index, space);
			break;
		case 'm':
			index = write_num(time->mon + 1, 2, index, space);
			break;
		case 'M':
			index = write_num(time->min, 2, index, space);
			break;
		case 'n':
			*index++ = '\n';
			break;
		case 'p':
			index = strncpy(index, time->hour > 11 ? "PM" : "AM", space);
			break;
		case 'P':
			index = strncpy(index, time->hour > 11 ? "pm" : "am", space);
			break;
		case 'q':
			index = write_num((time->mon + 3) / 3, 0, index, space);
			break;
		case 'S':
			index = write_num(time->sec, 2, index, space);
			break;
		case 't':
			*index++ = '\t';
			break;
		case 'u':
			index = write_num(((time->wday + 1) % 7) + 1, 0, index, space);
			break;
		case 'w':
			index = write_num(time->wday, 0, index, space);
			break;
		case 'y':
			index = write_num(time->yn, 2, index, space);
			break;
		case 'Y':
			index = write_num(time->year + 1900, 0, index, space);
			break;
		default: {
			char b[3] = { '%', c, '\0' };
			index = strncpy(index, b, space);
			break;
		}
		}
	}

	if (space < 1)
		buf[n - 1] = '\0';
	else
		*index = '\0';
}
