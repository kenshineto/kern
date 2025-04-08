#include <lib.h>

volatile void *memsetv(volatile void *dest, int c, size_t n)
{
	volatile unsigned char *d = dest;
	for (; n; n--) {
		*d++ = c;
	};
	return dest;
}
