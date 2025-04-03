#include <stdlib.h>

unsigned int bound(unsigned int min, unsigned int value, unsigned int max)
{
	if (value < min) {
		value = min;
	}
	if (value > max) {
		value = max;
	}
	return value;
}
