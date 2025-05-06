/**
 * @file pit.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * Programmable Interrupt Timer
 */

#ifndef PIT_H_
#define PIT_H_

#define CHAN_TIMER 0x40
#define CHAN_SPKR 0x42

#include <stdint.h>

// how many time the pit has ticked
// not accurate time, good for spinning though
extern volatile uint64_t ticks;

/**
 * Read timer frequency
 */
uint32_t pit_read_freq(uint8_t chan);

/**
 * Set timer frequency
 */
void pit_set_freq(uint8_t chan, uint32_t hz);

#endif
