#include <comus/cpu.h>
#include <comus/memory.h>
#include <comus/mboot.h>
#include <comus/drivers.h>
#include <comus/fs.h>
#include <lib.h>

void main(long magic, volatile void *mboot)
{
	// initalize idt and pic
	cpu_init();

	// load multiboot information
	mboot_init(magic, mboot);

	// initalize memory
	memory_init();

	// initalize devices
	drivers_init();

	// load file systems
	fs_init();

	// halt
	kprintf("halting...\n");
}
