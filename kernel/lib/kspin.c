#include <lib.h>
#include <comus/drivers/pit.h>
#include <comus/asm.h>

void kspin_seconds(size_t seconds)
{
	kspin_milliseconds(seconds * 1000);
}

void kspin_milliseconds(size_t milliseconds)
{
	uint64_t start = ticks;
	while ((ticks - start) < milliseconds)
		int_wait();
}
