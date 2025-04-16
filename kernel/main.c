#include "lib/kio.h"
#include <comus/cpu.h>
#include <comus/memory.h>
#include <comus/mboot.h>
#include <comus/drivers.h>
#include <comus/drivers/acpi.h>
#include <comus/drivers/pci.h>
#include <comus/fs.h>
#include <lib.h>

void kreport(void)
{
	cpu_report();
	memory_report();
	acpi_report();
	pci_report();
}

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

	// report system state
	kreport();

	// halt
	kprintf("halting...\n");
}
