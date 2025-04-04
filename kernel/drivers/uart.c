#include <comus/drivers/uart.h>
#include <comus/asm.h>

#define PORT 0x3F8

int uart_init(void)
{
	outb(PORT + 1, 0x00); // disable interrupts
	outb(PORT + 3, 0x80); // enable DLAB (divisor latch access bit)
	outb(PORT + 0, 0x03); // (lo) Set baud divisor to 3 38400 baud
	outb(PORT + 1, 0x00); // (hi)
	outb(PORT + 3,
		 0x03); // disable DLAB, set 8 bits per word, one stop bit, no parity
	outb(PORT + 2, 0xC7); // enable and clear FIFOs, set to maximum threshold
	outb(PORT + 4, 0x0B); // ???
	outb(PORT + 4, 0x1E); // set in loopback mode for test
	outb(PORT + 0, 0xAE); // test by sending 0xAE

	uint8_t response = inb(PORT + 0);
	if (response != 0xAE) {
		// TODO: panic here?
		return -1;
	}

	// disable loopback, enable IRQs
	outb(PORT + 4, 0x0F);
	return 0;
}

uint8_t uart_in(void)
{
	// wait for data to be available
	while ((inb(PORT + 5) & 0x01) == 0)
		;
	return inb(PORT);
}

void uart_out(uint8_t ch)
{
	// wait for output to be free
	while ((inb(PORT + 5) & 0x20) == 0)
		;
	outb(PORT, ch);
}

void uart_out_str(const char *str)
{
	while (*str)
		uart_out(*str++);
}
