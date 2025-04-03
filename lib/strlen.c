#include <string.h>

size_t strlen(const char *str)
{
	const char *p;
	for (p = str; *p != 0; p++) {
	}
	return p - str;
}
