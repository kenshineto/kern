ENTRY(_start)

SECTIONS
{
	. = 1M;

	kernel_start = .;

	.boot : {
		*(.multiboot)
	}

	. = ALIGN(0x1000);

	.rodata : {
		*(.rodata .rodata.* .gnu.linkonce.r.*)
	}

	. = ALIGN(0x1000);

	.text : {
		*(.text .stub .text.* .gnu.linkonce.t.*)
	}

	. = ALIGN(0x1000);

	.bss : {
		*(COMMON)
		*(.bss .bss.*)
	}

	/DISCARD/ : {
		*(.stab .stab_info .stabstr)
		*(.eh_frame .eh_frame_hdr)
		*(.note.GNU-stack .note.gnu.property .comment)
	}

	kernel_end = .;
}
