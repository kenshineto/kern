#include <lib.h>
#include <comus/drivers/tty.h>
#include <comus/drivers/uart.h>

void fputc(FILE *stream, char c)
{
	(void)stream; // TODO: manage stream
	uart_out(c);
	tty_out(c);
}

