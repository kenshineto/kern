#include <lib.h>

int ctoi(char c)
{
	if (c >= '0' && c <= '9') {
		return c - '0';
	} else if (c >= 'A' && c <= 'Z') {
		return c - 'A' + 10;
	} else if (c >= 'a' && c <= 'z') {
		return c - 'a' + 10;
	} else {
		return -1;
	}
}
