/**
  * @file vga.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * VGA Text Mode Functions
 */

#ifndef VGA_H_
#define VGA_H_

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

#include <stdint.h>

/**
 * Draw a character to the vga text mode at a given position
 */
void vga_draw_char(char c, uint16_t x, uint16_t y);

#endif /* vga.h */
