/*
** Simple linker script for user-level programs.
*/

OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS
{
	/* user text begins at the second page of the address space */
	. = 0x1000;

	.text : {
		KEEP(*(.text .stub .text.* .gnu.linkonce.t.*))
	}

	/* define some standard symbols */
	PROVIDE(etext = .);
	PROVIDE(_etext = .);

	/* read-only data will go at the end of the text section */
	.rodata : {
		KEEP(*(.rodata .rodata.* .gnu.linkonce.r.*))
	}

	/* Align the data segment at the next page boundary */
	. = ALIGN(0x1000);

	.data : {
		KEEP(*(.data))
	}

	PROVIDE(edata = .);
	PROVIDE(_edata = .);

	/* Page-align the BSS segment */
	. = ALIGN(0x1000);

	PROVIDE(__bss_start = .);

	.bss : {
		KEEP(*(.bss))
	}

	PROVIDE(_end = .);

	/DISCARD/ : {
		*(.stab .stab_info .stabstr .eh_frame .note.GNU-stack .note.gnu.property .comment)
	}
}
