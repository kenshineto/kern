#include <comus/cpu.h>
#include <comus/memory.h>
#include <comus/mboot.h>
#include <comus/drivers.h>
#include <lib.h>
#include <stdio.h>
#include <time.h>

struct memory_map mmap;

void main(long magic, volatile void *mboot)
{
	(void)magic; // TODO: check multiboot magic

	// initalize idt and pic
	// WARNING: must be done before anything else
	cpu_init();

	// load memory map
	mboot_load_mmap(mboot, &mmap);

	// initalize memory
	memory_init(&mmap);

	// initalize devices
	drivers_init();

	// print current time
	char date[40];
	set_timezone(TZ_EDT);
	time_t time = get_localtime();
	timetostr(&time, "%a %b %d %Y %H:%M:%S", date, 40);
	printf("The date is: %s\n\n", date);

	// halt
	printf("halting...\n");
}
