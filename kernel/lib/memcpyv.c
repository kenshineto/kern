#include <lib.h>

volatile void *memcpyv(volatile void *restrict dest,
					   const volatile void *restrict src, size_t n)
{
	volatile char *d = dest;
	volatile const char *s = src;
	for (; n; n--)
		*d++ = *s++;
	return dest;
}
