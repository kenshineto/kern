#include <lib.h>

int isspace(int c)
{
	switch (c) {
	case ' ':
	case '\t':
	case '\v':
	case '\f':
	case '\r':
	case '\n':
		return 1;
	default:
		return 0;
	}
}
