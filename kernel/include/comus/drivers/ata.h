#ifndef ATA_H_
#define ATA_H_

/*
 * @file ata.h
 *
 * @author Ian McFarlane <i.mcfarlane2002@gmail.com>
 * @author Freya Murphy <freya@freyacat.org>
 *
 * ATA driver
 */

#include <stdbool.h>

/**
 * @returns 0 on success, 1 on failure
 */
int ata_init(void);

/**
 * report all ata devices to console
 */
void ata_report(void);

#endif
