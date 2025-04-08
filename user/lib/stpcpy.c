#include <string.h>

char *stpcpy(char *restrict dest, const char *restrict src)
{
	char *d = dest;
	for (; (*d = *src); d++, src++)
		;
	return d;
}
