ENTRY(_start)

PHDRS
{
   text PT_LOAD;
   rodata PT_LOAD;
   data PT_LOAD;
}

SECTIONS
{
	. = 1M;

	kernel_start = .;

	. = ALIGN(0x1000);

	.text : {
		text_start = .;
		*(.multiboot)
		*(.text)
		*(.text.*)
		text_end = .;
	} : text

	. = ALIGN(0x1000);

	.rodata : {
		rodata_start = .;
		*(.rodata)
		*(.rodata.*)
		rodata_end = .;
	} : rodata

	. = ALIGN(0x1000);

	.data : {
		data_start = .;
		*(.data)
		*(.data.*)
		data_end = .;
	} : data

	. = ALIGN(0x1000);

	.bss : {
		bss_start = .;
		*(COMMON)
		*(.bss)
		*(.bss.*)
		bss_end = .;
	} : data

	kernel_end = .;

	/DISCARD/ : {
		*(.eh_frame .note.GNU-stack .note.gnu.property .comment)
	}
}
