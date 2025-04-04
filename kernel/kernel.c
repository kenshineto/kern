#include <comus/cpu.h>
#include <comus/memory.h>
#include <comus/mboot.h>
#include <lib.h>
#include <stdio.h>
#include <stdlib.h>

struct memory_map mmap;

void main(long magic, volatile void *mboot)
{
	(void) magic; // TODO: check multiboot magic

	// initalize idt and pic
	// WARNING: must be done before anything else
	cpu_init();

	// load memory map
	mboot_load_mmap(mboot, &mmap);

	// initalize memory
	memory_init(&mmap);

	char *a = malloc(3);
	*a = 3;

	// halt
	printf("halting...\n");
}
