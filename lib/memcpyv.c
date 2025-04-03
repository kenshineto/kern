#include <string.h>

void *memcpyv(volatile void *restrict dest, const volatile void *restrict src,
			  size_t n)
{
	char *d = dest;
	const char *s = src;
	for (; n; n--)
		*d++ = *s++;
	return dest;
}
