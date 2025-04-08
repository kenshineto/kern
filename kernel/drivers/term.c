#include <lib.h>
#include <comus/drivers/term.h>
#include <comus/asm.h>
#include <comus/memory.h>

#define VGA_ADDR 0xB8000

static const uint8_t width = 80;
static const uint8_t height = 25;
static volatile uint16_t *buffer = (uint16_t *)VGA_ADDR;

// position
static uint32_t x = 0, y = 0;

// color
static uint8_t fg = 15, bg = 0;

// blank color
const uint16_t blank = (uint16_t)0 | 0 << 12 | 15 << 8;

void term_clear_line(int line)
{
	if (line < 0 || line >= height)
		return;
	for (uint8_t x = 0; x < width; x++) {
		const size_t index = line * width + x;
		buffer[index] = blank;
	}
}

void term_clear(void)
{
	for (uint8_t y = 0; y < height; y++)
		term_clear_line(y);
}

void term_scroll(int lines)
{
	cli();
	y -= lines;
	if (!lines)
		return;
	if (lines >= height || lines <= -height) {
		term_clear();
	} else if (lines > 0) {
		memmovev(buffer, buffer + lines * width, 2 * (height - lines) * width);
		term_clear_line(height - lines);
	} else {
		memmovev(buffer + lines * width, buffer + lines,
				 (height + lines) * width);
	}
	sti();
}

void term_out(char c)
{
	if (buffer == NULL)
		return;

	// handle special chars
	switch (c) {
	case '\n':
		x = 0;
		y++;
		break;
	case '\t':
		x += 4;
		break;
	case '\v':
	case '\f':
		y++;
		break;
	case '\r':
		x = 0;
		break;
	default: {
		const size_t index = y * width + x;
		buffer[index] = c | bg << 12 | fg << 8;
		x++;
	}
	}

	// wrap cursor if needed
	if (x >= width) {
		x = 0;
		y++;
	}
	if (y >= height) {
		term_scroll(y - (height - 1));
		y = height - 1;
	}

	// set cursor position on screen
	const uint16_t pos = y * width + x;
	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t)(pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void term_out_str(const char *str)
{
	while (*str)
		term_out(*str++);
}
