/**
 * @file pic.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * PIC functions
 */

#ifndef PIC_H_
#define PIC_H_

#define PIC_REMAP_OFFSET 0x20

/**
 * Remaps the pie, i.e. initializes it
 */
void pic_remap(void);

/**
 * Masks an external irq to stop firing until un masked
 * @param irq - the irq to mask
 */
void pic_mask(int irq);

/**
 * Unmasks an external irq to allow interrupts to continue for that irq
 * @param irq - the irq to unmask
 */
void pic_unmask(int irq);

/**
 * Disabled the pick
 */
void pic_disable(void);

/**
 * Tells the pick that the interrupt has ended
 * @param irq - the irq that has ended
 */
void pic_eoi(int irq);

#endif /* pic.h */
