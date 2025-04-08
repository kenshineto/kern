#include <string.h>

int strncmp(const char *restrict lhs, const char *restrict rhs, size_t n)
{
	const unsigned char *l = (void *)lhs, *r = (void *)rhs;
	if (!n--)
		return 0;
	for (; *l && *r && n && *l == *r; l++, r++, n--)
		;
	return *l - *r;
}
