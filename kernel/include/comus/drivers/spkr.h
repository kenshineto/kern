/**
 * @file spkr.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * PC Speaker
 */

#ifndef SPKR_H_
#define SPKR_H_

#include <stdint.h>

/**
 * Play a tone on the pc speaker continuously
 */
void spkr_play_tone(uint32_t hz);

/**
 * Shut up the pc speaker
 */
void spkr_quiet(void);

/**
 * Beep the pc speaker for a short period
 */
void spkr_beep(void);

#endif /* spkr.h */
