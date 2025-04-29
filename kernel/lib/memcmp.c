#include <lib.h>

int memcmp(const void *restrict vl, const void *restrict vr, register size_t n)
{
	const unsigned char *l = vl, *r = vr;
	for (; n && *l == *r; n--, l++, r++)
		;
	return n ? *l - *r : 0;
}
