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

#define ATA_SECT_SIZE 512

#include <stdint.h>
#include <stdbool.h>

// type intended to be opaque- may be made into a struct in the future
typedef uint8_t ide_device_t;

enum ide_error {
	IDE_ERROR_OK = 0,
	IDE_ERROR_NULL_DEVICE, // device doesnt exist
	IDE_ERROR_INIT_NO_IDE_CONTROLLER,
	IDE_ERROR_INIT_BAD_HEADER,
	IDE_ERROR_POLL_DRIVE_REQUEST_NOT_READY,
	IDE_ERROR_POLL_DEVICE_FAULT,
	IDE_ERROR_POLL_STATUS_REGISTER_ERROR,
	IDE_ERROR_POLL_WRITE_PROTECTED,
};

struct ide_devicelist {
	ide_device_t devices[4];
	uint8_t num_devices;
};

/**
 * @returns 0 on success, 1 on failure
 */
enum ide_error ata_init(void);

/**
 * reads a number of sectors from the provided IDE/ATA device
 *
 * @returns IDE_ERROR_OK (0) on success or an error code on failure
 */
enum ide_error ide_device_read_sectors(ide_device_t, uint8_t numsects,
									   uint32_t lba,
									   uint16_t buf[numsects * 256]);

/**
 * writes a number of sectors to the provided IDE/ATA device
 *
 * @returns 0 on success or an error code on failure
 */
enum ide_error ide_device_write_sectors(ide_device_t, uint8_t numsects,
										uint32_t lba,
										uint16_t buf[numsects * 256]);

/*
 * Returns a variable number (between 0-4, inclusive)  of ide_devic_t's.
 */
struct ide_devicelist ide_devices_enumerate(void);

/**
 * report all ata devices to console
 */
void ata_report(void);

#endif
