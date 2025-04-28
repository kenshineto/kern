#include <lib.h>

void *memcpy(void *restrict dest, const void *restrict src, register size_t n)
{
	char *d = dest;
	const char *s = src;
	for (; n; n--)
		*d++ = *s++;
	return dest;
}
