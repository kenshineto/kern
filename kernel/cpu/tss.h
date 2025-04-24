/**
 * @file tss.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * TSS functions
 */

#ifndef TSS_H_
#define TSS_H_

#define TSS_REMAP_OFFSET 0x20

#include <stdint.h>

/**
 * Load the TSS selector
 */
void tss_init(void);

/**
 * Flush the tss
 */
void tss_flush(void);

/**
 * Set the kernel stack pointer in the tss
 */
void tss_set_stack(uint64_t stack);

#endif /* tss.h */
