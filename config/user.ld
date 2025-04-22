ENTRY(_start)

PHDRS
{
   text PT_LOAD;
   rodata PT_LOAD;
   data PT_LOAD;
}

SECTIONS
{
	. = 1024M;

	user_start = .;

	.text : {
		text_start = .;
		*(.text)
		*(.text.*)
		*(.gnu.linkonce.t.*)
		text_end = .;
	} : text

	. = ALIGN(0x1000);

	.rodata : {
		rodata_start = .;
		*(.rodata)
		*(.rodata.*)
		*(.gnu.linkonce.r.*)
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

	user_end = .;

	/DISCARD/ : {
		*(.eh_frame .note.GNU-stack .note.gnu.property .comment)
	}
}
