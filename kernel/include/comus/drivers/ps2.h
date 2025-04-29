/**
 * @file ps2.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * PS/2 Mouse & Keyboard
 */

#ifndef PS2_H_
#define PS2_H_

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

#endif /* ps2.h */
