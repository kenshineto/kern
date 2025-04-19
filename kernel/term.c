#include <lib.h>
#include <comus/term.h>
#include <comus/asm.h>
#include <comus/limits.h>
#include <comus/drivers/gpu.h>

// terminal data
static char buffer[TERM_MAX_WIDTH * TERM_MAX_HEIGHT];
static uint16_t buffer_line = UINT16_MAX;
static uint16_t width = 80; // baseline vga text mode until resized
static uint16_t height = 25;
static uint16_t x = 0;
static uint16_t y = 0;

#define BUFY(y) ((buffer_line + (y)) % TERM_MAX_HEIGHT)
#define BUFIDX(x, y) ((x) + (BUFY(y) * TERM_MAX_WIDTH))

static void buffer_check(void)
{
	if (buffer_line == UINT16_MAX) {
		buffer_line = 0;
		memset(buffer, 0, sizeof(buffer));
	}
}

static inline bool printable(char c)
{
	switch (c) {
	case '\0':
	case '\n':
	case '\t':
	case '\v':
	case '\f':
	case '\r':
		return false;
	default:
		return true;
	}
}

static inline void term_move_cur(char c)
{
	if (c == '\0')
		return;

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
	default:
		x++;
	}

	if (x >= width) {
		x = 0;
		y++;
	}
	if (y >= height) {
		term_scroll(y - (height - 1));
		y = height - 1;
	}
}

uint16_t term_width(void)
{
	return width;
}

uint16_t term_height(void)
{
	return height;
}

void term_out(char c)
{
	term_out_at(c, x, y);
	term_move_cur(c);
}

void term_out_at(char c, uint16_t x, uint16_t y)
{
	buffer_check();

	if (!printable(c))
		return;

	buffer[BUFIDX(x, y)] = c;
	gpu_draw_char(c, x, y);
}

void term_out_str(const char *str)
{
	char c;
	while (c = *(str++), c) {
		term_out_at(c, x, y);
		term_move_cur(c);
	}
}

void term_out_str_at(const char *str, uint16_t x, uint16_t y)
{
	char c;
	while (c = *(str++), c)
		term_out_at(c, x, y);
}

void term_clear(void)
{
	for (uint16_t y = 0; y < height; y++)
		term_clear_line(y);
}

void term_clear_line(uint16_t line)
{
	buffer_check();

	if (line > height)
		return;

	for (uint16_t x = 0; x < width; x++) {
		buffer[BUFIDX(x, line)] = 0;
		gpu_draw_char(0, x, line);
	}
}

void term_redraw(void)
{
	buffer_check();

	for (uint16_t j = 0; j < height; j++) {
		for (uint16_t i = 0; i < width; i++) {
			char c = buffer[BUFIDX(i, j)];
			gpu_draw_char(c, i, j);
		}
	}
}

void term_scroll(uint16_t lines)
{
	if (!lines)
		return;
	buffer_line += lines;
	term_redraw();
}

void term_resize(uint16_t w, uint16_t h)
{
	width = w % TERM_MAX_WIDTH;
	height = h % TERM_MAX_HEIGHT;
	term_redraw();
}
