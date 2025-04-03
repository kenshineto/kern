#include <stdlib.h>

static char suffixes[] = { 'K', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y', 'R', 'Q' };

char *btoa(size_t bytes, char *buf)
{
	// no suffix if under 1K, print up to four digits
	if (bytes < 1024) {
		ultoa(bytes, buf, 10);
		return buf;
	}

	// store one digit of remainder for decimal
	unsigned int remainder;
	// power of 1024
	int power = 0;

	// iterate until remaining bytes fits in three digits
	while (bytes >= 1000) {
		remainder = (bytes % 1024) * 10 / 1024;
		bytes /= 1024;
		power += 1;
	}

	// end of number
	char *end;

	if (bytes >= 10) {
		// no decimal
		end = ultoa(bytes, buf, 10);
	} else {
		// decimal
		end = ultoa(bytes, buf, 10);
		end[0] = '.';
		end = ultoa(remainder, end + 1, 10);
	}

	// add suffix
	end[0] = suffixes[power - 1];
	end[1] = '\0';

	return buf;
}
