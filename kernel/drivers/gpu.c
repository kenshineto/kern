#include <lib.h>
#include <comus/drivers/gpu.h>
#include <comus/drivers/gpu/gop.h>
#include <comus/drivers/gpu/bochs.h>
#include <comus/drivers/gpu/vga_text.h>
#include <comus/error.h>
#include <comus/term.h>
#include <comus/asm.h>

struct gpu *gpu_dev = NULL;

int gpu_init(void)
{
	// try to get a gpu device
	if (!gpu_dev && gop_init(&gpu_dev) == SUCCESS) {
	}
	if (!gpu_dev && bochs_init(&gpu_dev) == SUCCESS) {
	}

	// if we did (yay!) resize terminal
	if (gpu_dev)
		term_resize(gpu_dev->width, gpu_dev->height);

	return gpu_dev != NULL;
}

void gpu_set_pixel(uint32_t x, uint32_t y, uint32_t r, uint32_t g, uint32_t b)
{
	// TODO: handle other bpp
	volatile uint32_t *fb = (volatile uint32_t *)gpu_dev->framebuffer;
	int offset = y * gpu_dev->width + x;
	fb[offset] = (b << 0) | (g << 8) | (r << 16);
}

void gpu_draw_char(char c, uint16_t cx, uint16_t cy)
{
	if (gpu_dev == NULL) {
		vga_text_draw_char(c, cx, cy);
		return;
	}

	uint32_t sx = en_font.width * cx;
	uint32_t sy = en_font.height * cy;

	uint8_t *glyph = &en_font.data[en_font.glyph_size * c];

	for (uint32_t i = 0; i < en_font.width; i++) {
		for (uint32_t j = 0; j < en_font.height; j++) {
			uint32_t x = sx + (en_font.width - i - 1);
			uint32_t y = sy + j;
			uint32_t bitoff = i + j * en_font.width;
			uint32_t byteoff = bitoff / 8;
			uint32_t bitshift = bitoff % 8;

			uint32_t color = ((glyph[byteoff] >> bitshift) & 0x1) * 255;
			gpu_set_pixel(x, y, color, color, color);
		}
	}
}

void gpu_report(void)
{
	if (gpu_dev == NULL)
		return;

	kprintf("GPU (%s)\n", gpu_dev->name);
	kprintf("Width: %d\n", gpu_dev->width);
	kprintf("Height: %d\n", gpu_dev->height);
	kprintf("Bit depth: %d\n\n", gpu_dev->bit_depth);
}
