#include <lib.h>
#include <stdio.h>
#include <comus/asm.h>

#define PORT 0x3F8
static void serial_out(uint8_t ch) {
	// wait for output to be free
	while ((inb(PORT + 5) & 0x20) == 0);
	outb(PORT, ch);
}

void fputc(FILE *stream, char c) {
	(void) stream;

	serial_out(c);
}
