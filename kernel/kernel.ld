/*
** Simple linker script for the 20245 kernel.
*/

OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS
{
	/* Link the kernel at this address. */
        /* Must match what is defined in vm.h! */
	. = 0x80010000;

	.text : AT(0x10000) {
		*(.text .stub .text.* .gnu.linkonce.t.*)
	}

	/* standard symbols */
	PROVIDE(etext = .);
	PROVIDE(_etext = .);

	/* put read-only data next */
	.rodata : {
		*(.rodata .rodata.* .gnu.linkonce.r.*)
	}

	/* Align the data segment at the next page boundary */
	/* . = ALIGN(0x1000); */

	PROVIDE(data = .);
	PROVIDE(_data = .);

	/* The data segment */
	.data : {
		*(.data)
	}

	PROVIDE(edata = .);
	PROVIDE(_edata = .);

	/* page-align the BSS */
	. = ALIGN(0x1000);

	PROVIDE(__bss_start = .);

	.bss : {
		*(.bss)
	}

	PROVIDE(end = .);
	PROVIDE(_end = .);

	/DISCARD/ : {
		*(.stab .stab_info .stabstr .eh_frame .note.GNU-stack .note.gnu.property .comment)
	}
}
