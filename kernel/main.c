#include <comus/cpu.h>
#include <comus/memory.h>
#include <comus/mboot.h>
#include <comus/drivers.h>
#include <comus/fs.h>
#include <lib.h>

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

	// load file systems
	fs_init();

	// halt
	kprintf("halting...\n");
}
