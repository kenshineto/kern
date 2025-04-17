#include <lib.h>
#include <comus/drivers/gpu.h>
#include <comus/drivers/gpu/bochs.h>
#include <comus/error.h>
#include <comus/term.h>
#include <comus/asm.h>

enum gpu_type {
	GPU_UNSET = 0,
	GPU_BOCHS = 1,
	N_GPU_TYPE,
};

static enum gpu_type type = GPU_UNSET;

static const char *gpu_type_str[N_GPU_TYPE] = { "Unknown", "Bochs" };

struct psf2_font {
	uint8_t magic[4];
	uint32_t version;
	uint32_t header_size;
	uint32_t flags;
	uint32_t length;
	uint32_t glyph_size;
	uint32_t height;
	uint32_t width;
	uint8_t data[];
};

extern struct psf2_font en_font;

int gpu_init(void)
{
	if (bochs_init() == SUCCESS)
		type = GPU_BOCHS;

	if (type != GPU_UNSET) {
		uint16_t width = gpu_width() / en_font.width;
		uint16_t height = gpu_height() / en_font.height;
		term_switch_handler(width, height, gpu_draw_char);
	}

	return type != GPU_UNSET;
}

uint32_t gpu_width(void)
{
	switch (type) {
	case GPU_BOCHS:
		return bochs_width();
	default:
		return 0;
	}
}

uint32_t gpu_height(void)
{
	switch (type) {
	case GPU_BOCHS:
		return bochs_height();
	default:
		return 0;
	}
}

uint8_t gpu_bit_depth(void)
{
	switch (type) {
	case GPU_BOCHS:
		return bochs_bit_depth();
	default:
		return 0;
	}
}

void gpu_set_pixel(uint32_t x, uint32_t y, uint32_t r, uint32_t g, uint32_t b)
{
	switch (type) {
	case GPU_BOCHS:
		bochs_set_pixel(x, y, r, g, b);
		break;
	default:
		break;
	}
}

void gpu_draw_char(char c, uint16_t cx, uint16_t cy)
{
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
	if (type == GPU_UNSET)
		return;

	kprintf("GPU (%s)\n", gpu_type_str[type]);
	kprintf("Width: %d\n", gpu_width());
	kprintf("Height: %d\n", gpu_height());
	kprintf("Bit depth: %d\n\n", gpu_bit_depth());
}
