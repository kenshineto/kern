#include <lib.h>

char *strcpy(char *restrict dest, const char *restrict src)
{
	char *d = dest;
	for (; (*d = *src); d++, src++)
		;
	return dest;
}
