#include <lib.h>

#define UXTOA(type, name)                                \
	char *name(unsigned type n, char *buffer, int radix) \
	{                                                    \
		if (n == 0) {                                    \
			buffer[0] = '0';                             \
			buffer[1] = '\0';                            \
			return buffer + 1;                           \
		}                                                \
		char *start = buffer;                            \
		for (; n; n /= radix) {                          \
			*buffer++ = itoc(n % radix);                 \
		}                                                \
		char *buf_end = buffer;                          \
		*buffer-- = '\0';                                \
		while (buffer > start) {                         \
			char tmp = *start;                           \
			*start++ = *buffer;                          \
			*buffer-- = tmp;                             \
		}                                                \
		return buf_end;                                  \
	}

UXTOA(int, utoa)
UXTOA(long int, ultoa)
UXTOA(long long int, ulltoa)
