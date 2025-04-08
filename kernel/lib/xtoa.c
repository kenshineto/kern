#include <lib.h>

#define XTOA(type, name)                        \
	char *name(type n, char *buffer, int radix) \
	{                                           \
		if (n == 0) {                           \
			buffer[0] = '0';                    \
			buffer[1] = '\0';                   \
			return buffer + 1;                  \
		}                                       \
		if (n < 0) {                            \
			*buffer++ = '-';                    \
			n = -n;                             \
		}                                       \
		char *start = buffer;                   \
		for (; n; n /= radix) {                 \
			*buffer++ = itoc(n % radix);        \
		}                                       \
		char *buf_end = buffer;                 \
		*buffer-- = '\0';                       \
		while (buffer > start) {                \
			char tmp = *start;                  \
			*start++ = *buffer;                 \
			*buffer-- = tmp;                    \
		}                                       \
		return buf_end;                         \
	}

XTOA(int, itoa)
XTOA(long int, ltoa)
XTOA(long long int, lltoa)
