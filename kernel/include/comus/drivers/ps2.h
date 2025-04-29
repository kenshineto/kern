/**
 * @file ps2.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * PS/2 Mouse & Keyboard
 */

#ifndef PS2_H_
#define PS2_H_

#include <comus/keycodes.h>
#include <stdbool.h>

struct mouse_event {
	bool updated;
	bool lmb;
	bool rmb;
	bool mmb;
	int relx;
	int rely;
};

/**
 * Initalize the ps2 controller
 */
int ps2_init(void);

/**
 * Recieve input from ps2 keyboard during interrupt
 */
void ps2kb_recv(void);

/**
 * Return last read keycode
 */
struct keycode ps2kb_get(void);

/**
 * Recieve input from ps2 mouse during interrupt
 */
void ps2mouse_recv(void);

/**
 * Return last read mouse event
 */
struct mouse_event ps2mouse_get(void);

#endif /* ps2.h */
