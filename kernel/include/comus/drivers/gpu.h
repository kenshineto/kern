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

/**
 * Loads any gpu graphics driver
 * @returns 0 on success
 */
int gpu_init(void);

/**
 * @returns the width of the framebuffer
 */
uint32_t gpu_width(void);

/**
 * @returns the height of the framebuffer
 */
uint32_t gpu_height(void);

/**
 * @returns the bit depth of the framebuffer
 */
uint8_t gpu_bit_depth(void);

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
