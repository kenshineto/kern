#include <string.h>

void *memcpy(void *restrict dest, const void *restrict src, size_t n)
{
	char *d = dest;
	const char *s = src;
	for (; n; n--)
		*d++ = *s++;
	return dest;
}
