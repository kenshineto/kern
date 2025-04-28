#include <lib.h>

volatile void *memmovev(volatile void *dest, const volatile void *src,
						register size_t n)
{
	volatile char *d = dest;
	volatile const char *s = src;

	if (d == s)
		return d;

	if (d < s) {
		for (; n; n--)
			*d++ = *s++;
	} else {
		while (n)
			n--, d[n] = s[n];
	}

	return dest;
}
