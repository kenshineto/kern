ENTRY(_start)

SECTIONS
{
	. = 1M;

	kernel_start = .;

	. = ALIGN(0x1000);

	.text : {
		*(.multiboot)
		*(.text)
	}

	. = ALIGN(0x1000);

	.rodata : {
		*(.rodata)
	}

	. = ALIGN(0x1000);

	.data : {
		*(.data)
	}

	. = ALIGN(0x1000);

	.bss : {
		*(COMMON)
		*(.bss)
	}

	kernel_end = .;
}
