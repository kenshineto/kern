/**
 * @file ps2.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * PS/2 Mouse & Keyboard
 */

#ifndef PS2_H_
#define PS2_H_

#include <stdint.h>

/**
 * Initalize the ps2 controller
 */
int ps2_init(void);

/**
 * Recieve input from ps2 keyboard during interrupt
 */
void ps2kb_recv(void);

/**
 * Recieve input from ps2 mouse during interrupt
 */
void ps2mouse_recv(void);

/**
 * Set ps2 led state
 *
 * Bits
 * ----
 * 0 - Scroll lock
 * 1 - Num Lock
 * 2 - Caps lock
 */
int ps2_set_leds(uint8_t state);

#endif /* ps2.h */
