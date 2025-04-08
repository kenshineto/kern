#include <lib.h>

char *strncpy(char *restrict dest, const char *restrict src, size_t n)
{
	char *d = dest;
	for (; (*d = *src) && n; d++, src++, n--)
		;
	memset(d, 0, n);
	return dest;
}
