/**
 * @file gpu.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * Graphics Driver Initalization
 */

#ifndef GPU_H_
#define GPU_H_

#include <stdint.h>

struct gpu_dev {
	const char *name;
	uint16_t width;
	uint16_t height;
	uint16_t bit_depth;
	volatile void *framebuffer;
};

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

extern struct gpu_dev *gpu_dev;
extern struct psf2_font en_font;

/**
 * Loads any gpu graphics driver
 * @returns 0 on success
 */
int gpu_init(void);

/**
 * Sets the pixel at a given position
 */
void gpu_set_pixel(uint32_t x, uint32_t y, uint32_t r, uint32_t g, uint32_t b);

/**
 * Draw a character on the gpu at a given (terminal) position
 */
void gpu_draw_char(char c, uint16_t x, uint16_t y);

/**
 * Reports the state of the gpu
 */
void gpu_report(void);

#endif /* gpu.h */
