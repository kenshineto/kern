#include <lib/klib.h>
#include <comus/time.h>

void kspin_sleep_seconds(size_t seconds)
{
	const uint64_t start = unixtime();

	while (1) {
		const uint64_t now = unixtime();

		if (now - start > seconds) {
			return;
		}
	}
}
