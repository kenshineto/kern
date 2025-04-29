#include <lib.h>

void *memset(void *dest, int c, register size_t n)
{
	unsigned char *d = dest;
	for (; n; n--) {
		*d++ = c;
	};
	return dest;
}
