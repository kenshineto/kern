OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(bootentry)

SECTIONS
{
	. = 0x0;
	.text : { *(.text .stub .text.*) }
}
