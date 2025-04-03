#include <string.h>

void *memsetv(volatile void *dest, int c, size_t n)
{
	unsigned char *d = dest;
	for (; n; n--) {
		*d++ = c;
	};
	return dest;
}
