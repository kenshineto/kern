/**
 * @file gop.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * UEFI Graphics Output Protocol Driver
 */

#ifndef GOP_H_
#define GOP_H_

#include <comus/drivers/gpu.h>

/**
 * Loads the uefi gop graphics driver
 * @returns 0 on success, NULL on error
 */
int gop_init(struct gpu_dev **gpu_dev);

#endif /* gop.h */
