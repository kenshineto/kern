#include <lib.h>

int isprint(int c)
{
	return ((unsigned)(c - 0x20) <= (0x7e - 0x20));
}
