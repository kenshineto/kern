#include "lib/klib.h"
#include <comus/asm.h>
#include <comus/drivers/pit.h>
#include <comus/drivers/spkr.h>
#include <lib.h>

#define CMD 0x43
#define SPKR 0x61

#define BASE 1193180

volatile uint64_t ticks = 0;

uint32_t pit_read_freq(uint8_t chan)
{
	uint16_t div = 0;
	cli();
	outb(CMD, 0); // clear bits
	div = inb(chan); // low byte
	div |= inb(chan) << 8; // highbyte
	sti();
	return div * BASE;
}

void pit_set_freq(uint8_t chan, uint32_t hz)
{
	uint16_t div = BASE / hz;
	cli();
	outb(CMD, 0xb6);
	outb(chan, div & 0xFF); // low byte
	outb(chan, (div & 0xFF00) >> 8); // high byte
	sti();
}

void spkr_play_tone(uint32_t hz)
{
	uint8_t reg;

	if (hz == 0) {
		spkr_quiet();
		return;
	}

	// set spkr freq
	pit_set_freq(CHAN_SPKR, hz);

	// enable spkr gate
	reg = inb(SPKR);
	if (reg != (reg | 0x3))
		outb(SPKR, reg | 0x3);
}

void spkr_quiet(void)
{
	uint8_t reg;
	reg = inb(SPKR) & 0xFC;
	outb(SPKR, reg);
}

void spkr_beep(void)
{
	spkr_play_tone(1000);
	kspin_milliseconds(100);
	spkr_quiet();
}
