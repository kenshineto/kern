/**
 * @file bochs.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * Bochs Graphics Driver
 */

#ifndef BOCHS_H_
#define BOCHS_H_

#include <stdint.h>

/**
 * Loads the bochs graphics driver
 * @returns 0 on success
 */
int bochs_init(void);

/**
 * @returns the width of the framebuffer
 */
uint32_t bochs_width(void);

/**
 * @returns the height of the framebuffer
 */
uint32_t bochs_height(void);

/**
 * @returns the bit depth of the framebuffer
 */
uint8_t bochs_bit_depth(void);

/**
 * Sets the pixel at a given position
 */
void bochs_set_pixel(uint32_t x, uint32_t y, uint32_t r, uint32_t g,
					 uint32_t b);

#endif /* bochs.h */
