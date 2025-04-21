/*
 * @file ata.h
 *
 * @author Ian McFarlane <i.mcfarlane2002@gmail.com>
 * @author Freya Murphy <freya@freyacat.org>
 *
 * ATA driver
 */

#ifndef ATA_H_
#define ATA_H_

#include <stdint.h>

struct ide_channel {
	uint16_t io_base;
	uint16_t control_base;
	uint16_t bus_master_ide_base;
	uint8_t no_interrupt;
};

struct ide_device {
	uint8_t is_reserved; // 0 (Empty) or 1 (This Drive really exists).
	uint8_t channel_idx; // 0 (Primary Channel) or 1 (Secondary Channel).
	uint8_t drive_idx; // 0 (Master Drive) or 1 (Slave Drive).
	uint16_t type; // 0: ATA, 1:ATAPI.
	uint16_t drive_signature;
	uint16_t features;
	uint32_t supported_command_sets;
	uint32_t size_in_sectors;
	uint8_t model_str[41];
};

extern struct ide_channel ide_channels[2];
extern struct ide_device ide_devices[4];

/**
 * @returns 0 on success, 1 on failure
 */
int ata_init(void);

/**
 * reads a number of sectors from the provided IDE/ATA device
 *
 * @returns 0 on success or an error code on failure
 */
int ide_device_read_sectors(struct ide_device *dev, uint8_t numsects,
							uint32_t lba, uint16_t buf[numsects * 256]);

/**
 * writes a number of sectors to the provided IDE/ATA device
 *
 * @returns 0 on success or an error code on failure
 */
int ide_device_write_sectors(struct ide_device *dev, uint8_t numsects,
							 uint32_t lba, uint16_t buf[numsects * 256]);

/**
 * report all ata devices to console
 */
void ata_report(void);

#endif
