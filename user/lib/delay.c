#include <stdlib.h>

void delay(int count)
{
	while (count-- > 0)
		for (int i = 0; i < 100000; i++)
			;
}
