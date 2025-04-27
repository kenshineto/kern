#include <comus/asm.h>
#include <comus/drivers/pit.h>

#define CHAN_0 0x40
#define CHAN_1 0x41
#define CHAN_2 0x42
#define CMD 0x43

volatile uint64_t ticks = 0;

uint16_t pit_read_divider(void)
{
	uint16_t count = 0;
	cli();
	outb(CMD, 0); // clear bits
	count = inb(CHAN_0); // low byte
	count |= inb(CHAN_0) << 8; // highbyte
	sti();
	return count;
}

void pit_set_divider(uint16_t count)
{
	(void)count;
	cli();
	outb(CHAN_0, count & 0xFF); // low byte
	outb(CHAN_0, (count & 0xFF00) >> 8); // high byte
	sti();
}
