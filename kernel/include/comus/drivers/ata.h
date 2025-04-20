#ifndef ATA_H_
#define ATA_H_

/*
 * @file ata.h
 *
 * @author Ian McFarlane <i.mcfarlane2002@gmail.com>
 *
 * ATA driver
 */

#include <stdbool.h>

/// Returns true if a PCE IDE device is connected and we will have disk space,
/// false on no disks found.
bool ata_init(void);

#endif
