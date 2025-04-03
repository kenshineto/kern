ENTRY(_start)

SECTIONS
{
	. = 1M;

	.text : {
		*(.text .stub .text.* .gnu.linkonce.t.*)
	}

	.rodata : {
		*(.rodata .rodata.* .gnu.linkonce.r.*)
	}

	. = ALIGN(0x1000);

	.data : {
		*(.data .data.*)
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
}
