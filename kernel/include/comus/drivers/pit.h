/**
 * @file pit.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * Programmable Interrupt Timer
 */

#ifndef PIT_H_
#define PIT_H_

#include <stdint.h>

// how many time the pit has ticked
// not accurate time, good for spinning though
extern volatile uint64_t ticks;

uint16_t pit_read_divider(void);
void pit_set_divider(uint16_t count);

#endif
