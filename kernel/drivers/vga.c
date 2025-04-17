#include <lib.h>
#include <comus/term.h>
#include <comus/asm.h>
#include <comus/memory.h>
#include <comus/drivers/vga.h>

#define VGA_ADDR 0xB8000
static volatile uint16_t *buffer = (uint16_t *)VGA_ADDR;

// color
static uint8_t fg = 15, bg = 0;

void vga_draw_char(char c, uint16_t x, uint16_t y)
{
	// output character
	const size_t index = y * VGA_WIDTH + x;
	buffer[index] = c | bg << 12 | fg << 8;

	// set cursor position on screen
	const uint16_t pos = y * VGA_HEIGHT + x;
	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t)(pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}
