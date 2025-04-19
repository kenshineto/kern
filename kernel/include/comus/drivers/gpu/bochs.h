/**
 * @file bochs.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * Bochs Graphics Driver
 */

#ifndef BOCHS_H_
#define BOCHS_H_

#include <comus/drivers/gpu.h>

/**
 * Loads the bochs graphics driver
 * @returns 0 on success, NULL on error
 */
int bochs_init(struct gpu **gpu_dev);

#endif /* bochs.h */
