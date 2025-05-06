#include <comus/cpu.h>
#include <comus/memory.h>
#include <comus/mboot.h>
#include <comus/efi.h>
#include <comus/drivers.h>
#include <comus/drivers/acpi.h>
#include <comus/drivers/pci.h>
#include <comus/drivers/gpu.h>
#include <comus/drivers/ata.h>
#include <comus/user.h>
#include <comus/fs.h>
#include <comus/procs.h>
#include <lib.h>

void kreport(void)
{
	cpu_report();
	memory_report();
	acpi_report();
	pci_report();
	ata_report();
	gpu_report();
}

void load_init(void)
{
	struct file_system *fs;
	struct file *file;
	const char *init_vector[] = { NULL };

	if (pcb_alloc(&init_pcb)) {
		WARN("failed to alloc init pcb");
		return;
	}

	// get root fs
	fs = fs_get_root_file_system();
	if (fs == NULL) {
		WARN("failed to get root fs");
		return;
	}

	// get init bin
	if (fs->open(fs, "bin/init", O_RDONLY, &file)) {
		WARN("cannot find init elf");
		return;
	}

	if (user_load(init_pcb, file, init_vector, kernel_mem_ctx)) {
		WARN("init elf failed to load! bad bad BAD!!");
		file->close(file);
		return;
	}

	// close file
	file->close(file);

	// schedule and dispatch init
	schedule(init_pcb);
	dispatch();
}

__attribute__((noreturn)) void main(long magic, volatile void *mboot)
{
	// initalize idt and pic
	cpu_init();

	// load multiboot information
	mboot_init(magic, mboot);

	// load efi structures
	efi_init(mboot_get_efi_hdl(), mboot_get_efi_st());

	// initalize memory
	memory_init();

	// initalize devices
	drivers_init();

	// load file systems
	fs_init();

	// initalize processes
	pcb_init();

	// report system state
	kreport();

	// load init process
	load_init();

	panic("failed to load init");
}
