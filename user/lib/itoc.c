#include <stdlib.h>

char itoc(int i)
{
	if (i < 10) {
		return '0' + i;
	} else {
		return 'a' + (i - 10);
	}
}
