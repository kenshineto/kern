#include <lib.h>

void *memmove(void *dest, const void *src, size_t n)
{
	char *d = dest;
	const char *s = src;

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
