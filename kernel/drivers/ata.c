// taken from https://wiki.osdev.org/PCI#Class_Codes
#define CLASS_MASS_STORAGE_CONTROLLER 0x1

#define SUBCLASS_SCSI_BUS_CONTROLLER 0x0
#define SUBCLASS_IDE_CONTROLLER 0x1
#define SUBCLASS_FLOPPY_DISK_CONTROLLER 0x2
#define SUBCLASS_IPI_BUS_CONTROLLER 0x3
#define SUBCLASS_RAID_CONTROLLER 0x4
#define SUBCLASS_ATA_CONTROLLER 0x5
#define SUBCLASS_SERIAL_ATA_CONTROLLER 0x6
#define SUBCLASS_SERIAL_ATTACHED_SCSI_CONTROLLER 0x7
#define SUBCLASS_NON_VOLATILE_MEMORY_CONTROLLER 0x8
#define SUBCLASS_OTHER 0x80

// from https://wiki.osdev.org/PCI_IDE_Controller#Detecting_a_PCI_IDE_Controller
#define IDE_PROG_IF_PRIMARY_CHANNEL_IS_PCI_NATIVE_FLAG 0x1
#define IDE_PROG_IF_PRIMARY_CHANNEL_CAN_SWITCH_TO_AND_FROM_PCI_NATIVE_FLAG 0x2
#define IDE_PROG_IF_SECONDARY_CHANNEL_IS_PCI_NATIVE_FLAG 0x3
#define IDE_PROG_IF_SECONDARY_CHANNEL_CAN_SWITCH_TO_AND_FROM_PCI_NATIVE_FLAG 0x4
#define IDE_PROG_IF_DMA_SUPPORT_FLAG 0x7

// clang-format off
// ATA statuses, read off of the command/status port
#define ATA_SR_BUSY                 0x80
#define ATA_SR_DRIVEREADY           0x40
#define ATA_SR_DRIVEWRITEFAULT      0x20
#define ATA_SR_DRIVESEEKCOMPLETE    0x10
#define ATA_SR_DRIVEREQUESTREADY    0x08
#define ATA_SR_CORRECTEDATA         0x04
#define ATA_SR_INDEX                0x02
#define ATA_SR_ERROR                0x01

// ATA errors, read off of the features/error port
#define ATA_ER_BADBLOCK             0x80
#define ATA_ER_UNCORRECTABLE        0x40
#define ATA_ER_MEDIACHANGED         0x20
#define ATA_ER_IDMARKNOTFOUND       0x10
#define ATA_ER_MEDIACHANGEREQUEST   0x08
#define ATA_ER_COMMANDABORTED       0x04
#define ATA_ER_TRACK0NOTFOUND       0x02
#define ATA_ER_NOADDRESSMARK        0x01

// ATA commands
#define ATA_CMD_READ_PIO          0x20
#define ATA_CMD_READ_PIO_EXT      0x24
#define ATA_CMD_READ_DMA          0xC8
#define ATA_CMD_READ_DMA_EXT      0x25
#define ATA_CMD_WRITE_PIO         0x30
#define ATA_CMD_WRITE_PIO_EXT     0x34
#define ATA_CMD_WRITE_DMA         0xCA
#define ATA_CMD_WRITE_DMA_EXT     0x35
#define ATA_CMD_CACHE_FLUSH       0xE7
#define ATA_CMD_CACHE_FLUSH_EXT   0xEA
#define ATA_CMD_PACKET            0xA0
#define ATA_CMD_IDENTIFY_PACKET   0xA1
#define ATA_CMD_IDENTIFY          0xEC

// ATAPI commands
#define ATAPI_CMD_READ            0xA8
#define ATAPI_CMD_EJECT           0x1B

// values in "identification space," part of the disk with info about the device
#define ATA_IDENT_DEVICETYPE   0
#define ATA_IDENT_CYLINDERS    2
#define ATA_IDENT_HEADS        6
#define ATA_IDENT_SECTORS      12
#define ATA_IDENT_SERIAL       20
#define ATA_IDENT_MODEL        54
#define ATA_IDENT_FEATURES     98
#define ATA_IDENT_FIELDVALID   106
#define ATA_IDENT_MAX_LBA      120
#define ATA_IDENT_COMMANDSETS  164
#define ATA_IDENT_MAX_LBA_EXT  200

// interface types for use when selecting drives
#define IDE_ATA        0x00
#define IDE_ATAPI      0x01
#define ATA_MASTER     0x00
#define ATA_SLAVE      0x01

// the parts of the "task file," offsets from channels BAR0 and/or BAR2.
// right hand side comments are for primary channel values
#define ATA_REG_DATA       0x00 // rw
#define ATA_REG_ERROR      0x01 // r
#define ATA_REG_FEATURES   0x01 // w
#define ATA_REG_SECCOUNT0  0x02 // rw
#define ATA_REG_LBA0       0x03 // rw
#define ATA_REG_LBA1       0x04 // rw
#define ATA_REG_LBA2       0x05 // rw
#define ATA_REG_HDDEVSEL   0x06 // rw, used to select drive in channel
#define ATA_REG_COMMAND    0x07 // w
#define ATA_REG_STATUS     0x07 // r
#define ATA_REG_SECCOUNT1  0x08
#define ATA_REG_LBA3       0x09
#define ATA_REG_LBA4       0x0A
#define ATA_REG_LBA5       0x0B
#define ATA_REG_CONTROL    0x0C // w, can also be written as BAR1 + 2
#define ATA_REG_ALTSTATUS  0x0C // r, also BAR1 + 2
#define ATA_REG_DEVADDRESS 0x0D // BAR1 + 3 ???? osdev doesn't know what this is

// Channels:
#define      ATA_PRIMARY      0x00
#define      ATA_SECONDARY    0x01

// Directions:
#define      ATA_READ      0x00
#define      ATA_WRITE     0x01
// clang-format on

#include <comus/drivers/ata.h>
#include <comus/drivers/pci.h>
#include <comus/asm.h>
#include <lib.h>

static struct ide_channel {
	uint16_t io_base;
	uint16_t control_base;
	uint16_t bus_master_ide_base;
	uint8_t no_interrupt;
} channels[2];

static volatile uint8_t ide_irq_invoked = 0;
// static uint8_t atapi_packet[12] = { 0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

struct ide_device {
	uint8_t is_reserved; // 0 (Empty) or 1 (This Drive really exists).
	uint8_t channel_idx; // 0 (Primary Channel) or 1 (Secondary Channel).
	uint8_t drive_idx; // 0 (Master Drive) or 1 (Slave Drive).
	uint16_t type; // 0: ATA, 1:ATAPI.
	uint16_t drive_signature;
	uint16_t features;
	uint32_t supported_command_sets;
	uint32_t size_in_sectors;
	char model_str[41]; // NOTE: originally uint8_t according to osdev
} ide_devices[4];

static struct ide_device *device(const uint8_t idx)
{
	assert(idx < (sizeof(ide_devices) / sizeof(struct ide_device)),
		   "out of bounds");
	return ide_devices + idx;
}

static struct ide_channel *channel(const uint8_t idx)
{
	assert(idx < (sizeof(channels) / sizeof(struct ide_channel)),
		   "out of bounds");
	return channels + idx;
}

static void ide_channel_write(struct ide_channel *channel, const uint8_t reg,
							  uint8_t data)
{
	const bool disable_interrupts = reg > 0x07 && reg < 0x0C;
	if (disable_interrupts) {
		ide_channel_write(channel, ATA_REG_CONTROL,
						  0x80 | channel->no_interrupt);
	}

	if (reg < 0x08) {
		outb(channel->io_base + reg - 0x00, data);
	} else if (reg < 0x0C) {
		outb(channel->io_base + reg - 0x06, data);
	} else if (reg < 0x0E) {
		outb(channel->control_base + reg - 0x0A, data);
        // someone on OSdev said this was the correct thing
        // https://wiki.osdev.org/Talk:PCI_IDE_Controller
		// outb(channel->control_base + reg - 0x0C, data);
	} else if (reg < 0x16) {
		outb(channel->bus_master_ide_base + reg - 0x0E, data);
	}

	if (disable_interrupts) {
		ide_channel_write(channel, ATA_REG_CONTROL, channel->no_interrupt);
	}
}

static uint8_t ide_channel_read(struct ide_channel *channel, const uint8_t reg)
{
	uint8_t result;

	const bool disable_interrupts = reg > 0x07 && reg < 0x0C;

	if (disable_interrupts) {
		ide_channel_write(channel, ATA_REG_CONTROL,
						  0x80 | channel->no_interrupt);
	}

	if (reg < 0x08) {
		result = inb(channel->io_base + reg - 0x00);
	} else if (reg < 0x0C) {
		result = inb(channel->io_base + reg - 0x06);
	} else if (reg < 0x0E) {
		result = inb(channel->control_base + reg - 0x0A);
	} else if (reg < 0x16) {
		result = inb(channel->bus_master_ide_base + reg - 0x0E);
	} else {
		assert(false, "invalid ide channel register %u", reg);
	}

	if (disable_interrupts) {
		ide_channel_write(channel, ATA_REG_CONTROL, channel->no_interrupt);
	}

	return result;
}

// TODO: fix stack-trashing in ide_read_id_space_buffer() so we can call
// functions there and dont have to do this garbage. but i dont understand why
// esp and es need to be overwritten in the first place so I'm afraid i'll break
// something - Ian
#define __insl_nocall(reg, buffer_uint32_ptr, quads)       \
	do {                                                   \
		for (uint32_t index = 0; index < quads; ++index) { \
			uint32_t out;                                  \
			__inl_nocall(reg, out);                        \
			buffer_uint32_ptr[index] = out;                \
		}                                                  \
	} while (0)

void ide_read_id_space_buffer(struct ide_channel *channel, uint8_t reg,
							  uint32_t *out_buffer,
							  size_t out_buffer_size_bytes)
{
	const bool disable_interrupts = reg > 0x07 && reg < 0x0C;

	const size_t quads = out_buffer_size_bytes / 4;
	assert(sizeof(uint32_t) == 4, "what.");
	assert(out_buffer_size_bytes % 4 == 0,
		   "id space buffer size not divisible by 4");

	if (disable_interrupts) {
		ide_channel_write(channel, ATA_REG_CONTROL,
						  0x80 | channel->no_interrupt);
	}

	/* WARNING: This code contains a serious bug. The inline assembly trashes ES and
    *           ESP for all of the code the compiler generates between the inline
    *           assembly blocks. Do not call functions here.
    */
	// __asm__ volatile("pushw %es; movw %ds, %ax; movw %ax, %es");
	// NOTE: can be fixed with this code?
	// __asm__ volatile("pushw %es; pushw %ax; movw %ds, %ax; movw %ax, %es; popw %ax;");

	if (reg < 0x08) {
		__insl_nocall(channel->io_base + reg - 0x00, out_buffer, quads);
	} else if (reg < 0x0C) {
		__insl_nocall(channel->io_base + reg - 0x06, out_buffer, quads);
	} else if (reg < 0x0E) {
		__insl_nocall(channel->control_base + reg - 0x0A, out_buffer, quads);
	} else if (reg < 0x16) {
		__insl_nocall(channel->bus_master_ide_base + reg - 0x0E, out_buffer,
					  quads);
	}

	// __asm__ volatile("popw %es;");

	if (disable_interrupts) {
		ide_channel_write(channel, ATA_REG_CONTROL, channel->no_interrupt);
	}
}

enum ide_channel_poll_error {
	POLL_ERROR_OK,
	POLL_ERROR_DRIVE_REQUEST_NOT_READY,
	POLL_ERROR_DEVICE_FAULT,
	POLL_ERROR_STATUS_REGISTER_ERROR,
	POLL_ERROR_WRITE_PROTECTED,
};

enum ide_channel_poll_error ide_channel_poll(struct ide_channel *channel,
											 bool advanced_check)
{
	// (I) Delay 400 nanosecond for BSY to be set:
	for (int i = 0; i < 4; i++) {
		ide_channel_read(
			channel,
			ATA_REG_ALTSTATUS); // Reading the Alternate Status port wastes 100ns; loop four times.
	}

	while (ide_channel_read(channel, ATA_REG_STATUS) & ATA_SR_BUSY)
		; // Wait for BSY to be zero.

	if (advanced_check) {
		uint8_t state = ide_channel_read(channel, ATA_REG_STATUS);

		if (state & ATA_SR_ERROR) {
			return POLL_ERROR_STATUS_REGISTER_ERROR;
		}

		if (state & ATA_SR_DRIVEWRITEFAULT) {
			return POLL_ERROR_DEVICE_FAULT;
		}

		// BSY = 0; DF = 0; ERR = 0 so we should check for DRQ now.
		if ((state & ATA_SR_DRIVEREQUESTREADY) == 0) {
			return POLL_ERROR_DRIVE_REQUEST_NOT_READY;
		}
	}

	return POLL_ERROR_OK;
}

uint8_t ide_device_print_poll_error(struct ide_device *dev,
									const enum ide_channel_poll_error err)
{
	uint8_t out_err = err;

	if (err == POLL_ERROR_OK) {
		return out_err;
	}

	ERROR("IDE ERROR:");
	if (err == POLL_ERROR_DEVICE_FAULT) {
		ERROR("\tDevice fault");
		out_err = 19;
	}

	else if (err == POLL_ERROR_STATUS_REGISTER_ERROR) {
		uint8_t status_reg =
			ide_channel_read(channel(dev->channel_idx), ATA_REG_ERROR);

		if (status_reg & ATA_ER_NOADDRESSMARK) {
			ERROR("\tNo address mark found");
			out_err = 7;
		}

		if (status_reg & ATA_ER_TRACK0NOTFOUND) {
			ERROR("\tNo media or media error");
			out_err = 3;
		}

		if (status_reg & ATA_ER_COMMANDABORTED) {
			ERROR("\tDrive aborted command");
			out_err = 20;
		}

		if (status_reg & ATA_ER_MEDIACHANGEREQUEST) {
			ERROR("\tNo media or media error");
			out_err = 3;
		}

		if (status_reg & ATA_ER_IDMARKNOTFOUND) {
			ERROR("\tID mark not found, unable to read ID space");
			out_err = 21;
		}

		if (status_reg & ATA_ER_MEDIACHANGED) {
			ERROR("\tNo media or media error");
			out_err = 3;
		}

		if (status_reg & ATA_ER_UNCORRECTABLE) {
			ERROR("\tUncorrectable data error");
			out_err = 22;
		}
		if (status_reg & ATA_ER_BADBLOCK) {
			ERROR("\tBad sectors");
			out_err = 13;
		}
	} else if (err == POLL_ERROR_DRIVE_REQUEST_NOT_READY) {
		ERROR("\tRead nothing, drive request not ready");
		out_err = 23;
	} else if (err == POLL_ERROR_WRITE_PROTECTED) {
		ERROR("\tWrite-protected");
		out_err = 8;
	}

	static const char *channel_displaynames[] = { "Primary", "Secondary" };
	static const char *device_displaynames[] = { "Master", "Slave" };

	const char *channel_displayname = channel_displaynames[dev->channel_idx];
	const char *device_displayname = device_displaynames[dev->drive_idx];

	ERROR("\t[%s %s] %s", channel_displayname, device_displayname,
		  dev->model_str);

	return err;
}

void ide_initialize(uint32_t BAR0, uint32_t BAR1, uint32_t BAR2, uint32_t BAR3,
					uint32_t BAR4)
{
	// 1- Detect I/O Ports which interface IDE Controller:
	channels[ATA_PRIMARY] = (struct ide_channel){
		.io_base = (BAR0 & 0xFFFFFFFC) + 0x1F0 * (!BAR0),
		.control_base = (BAR1 & 0xFFFFFFFC) + 0x3F6 * (!BAR1),
		.bus_master_ide_base = (BAR4 & 0xFFFFFFFC) + 0,
	};
	channels[ATA_SECONDARY] = (struct ide_channel){
		.io_base = (BAR2 & 0xFFFFFFFC) + 0x170 * (!BAR2),
		.control_base = (BAR3 & 0xFFFFFFFC) + 0x376 * (!BAR3),
		.bus_master_ide_base = (BAR4 & 0xFFFFFFFC) + 8,
	};

	// 2- Disable IRQs:
	ide_channel_write(channel(ATA_PRIMARY), ATA_REG_CONTROL, 2);
	ide_channel_write(channel(ATA_SECONDARY), ATA_REG_CONTROL, 2);

	// 3- Detect ATA-ATAPI Devices:
	uint32_t device_count = 0;
	for (uint8_t channel_idx = 0; channel_idx < 2; channel_idx++)
		// drive idx is like device_count but it starts at 0 per channel
		// and increments regardless of whether a device is present
		for (uint8_t drive_idx = 0; drive_idx < 2; drive_idx++) {
			// select our uninitialized device from preallocated buffer
			struct ide_device *dev = device(device_count);

			// select our initialized channel
			struct ide_channel *chan = channel(channel_idx);

			// default drive to not existing
			dev->is_reserved = 0;

			// select drive:
			ide_channel_write(chan, ATA_REG_HDDEVSEL, 0xA0 | (drive_idx << 4));
			// This function should be implemented in your OS. which waits for 1 ms.
			// it is based on System Timer Device Driver.
			// sleep(1); // Wait 1ms for drive select to work.
			kspin_sleep_seconds(1); // TODO: sleep 1ms, this is way too long

			// (II) Send ATA Identify Command:
			ide_channel_write(chan, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
			// sleep(1);
			kspin_sleep_seconds(1); // TODO: sleep 1ms

			// (III) Polling:
			if (ide_channel_read(chan, ATA_REG_STATUS) == 0) {
				continue; // If status == 0, no device.
			}

			bool ata_err = false;
			while (1) {
				uint8_t status;
				status = ide_channel_read(chan, ATA_REG_STATUS);

				// if error, device is not ATA
				if ((status & ATA_SR_ERROR)) {
					ata_err = true;
					break;
				}

				// break when not busy and we our request is done
				if (!(status & ATA_SR_BUSY) &&
					(status & ATA_SR_DRIVEREQUESTREADY)) {
					break;
				}
			}

			// probe for ATAPI devices, if needed
			uint8_t type = IDE_ATA;
			if (ata_err) {
				uint8_t cl = ide_channel_read(chan, ATA_REG_LBA1);
				uint8_t ch = ide_channel_read(chan, ATA_REG_LBA2);

				if (cl == 0x14 && ch == 0xEB) {
					type = IDE_ATAPI;
				} else if (cl == 0x69 && ch == 0x96) {
					type = IDE_ATAPI;
				} else {
					// unknown type (may not be a device).
					continue;
				}

				ide_channel_write(chan, ATA_REG_COMMAND,
								  ATA_CMD_IDENTIFY_PACKET);
				// sleep(1);
				kspin_sleep_seconds(1); // TODO: sleep one millisecond
			}

			static uint8_t id_space_buf[2048] = { 0 };
			ide_read_id_space_buffer(chan, ATA_REG_DATA,
									 (uint32_t *)id_space_buf, 512);

			// read device info from the id space buffer into our struct:
			*dev = (struct ide_device){
				.is_reserved = 1,
				.type = type,
				.channel_idx = channel_idx,
				.drive_idx = drive_idx,
				.drive_signature =
					*((uint16_t *)(id_space_buf + ATA_IDENT_DEVICETYPE)),
				.features = *((uint16_t *)(id_space_buf + ATA_IDENT_FEATURES)),
				.supported_command_sets =
					*((uint32_t *)(id_space_buf + ATA_IDENT_COMMANDSETS))
			};

			// get size (depends on address mode):
			if (dev->supported_command_sets & (1 << 26)) {
				// device uses 48-bit addressing:
				dev->size_in_sectors =
					*((uint32_t *)(id_space_buf + ATA_IDENT_MAX_LBA_EXT));
			} else {
				// device uses CHS or 28-bit dddressing:
				dev->size_in_sectors =
					*((uint32_t *)(id_space_buf + ATA_IDENT_MAX_LBA));
			}

			// string indicates model of device (Western Digital HDD, SONY DVD-RW...)
			for (uint8_t i = 0; i < 40; i += 2) {
				dev->model_str[i] = id_space_buf[ATA_IDENT_MODEL + i + 1];
				dev->model_str[i + 1] = id_space_buf[ATA_IDENT_MODEL + i];
			}
			dev->model_str[40] = 0; // null-terminate string

			device_count++;
		}
}

uint8_t ide_device_ata_access(struct ide_device *dev, uint8_t direction,
							  uint32_t lba, uint8_t numsects, uint16_t selector,
							  uint32_t edi)
{
	bool dma = false; // TODO: support DMA
	bool cmd = false;
	uint8_t lba_mode /* 0: CHS, 1:LBA28, 2: LBA48 */;
	uint8_t lba_io[6];
	struct ide_channel *chan = channel(dev->channel_idx); // Read the Channel.
	uint32_t slavebit = dev->drive_idx; // Read the Drive [Master/Slave]
	uint32_t bus =
		chan->io_base; // Bus Base, like 0x1F0 which is also data port.
	uint32_t words =
		256; // Almost every ATA drive has a sector-size of 512-byte.
	uint16_t cyl, i;
	uint8_t head, sect, err;

	// disable irqs because we are using polling
	ide_channel_write(chan, ATA_REG_CONTROL,
					  chan->no_interrupt = (ide_irq_invoked = 0x0) + 0x02);

	// (I) Select one from LBA28, LBA48 or CHS;
	if (lba >=
		0x10000000) { // Sure Drive should support LBA in this case, or you are
		// giving a wrong LBA.
		// LBA48:
		lba_mode = 2;
		lba_io[0] = (lba & 0x000000FF) >> 0;
		lba_io[1] = (lba & 0x0000FF00) >> 8;
		lba_io[2] = (lba & 0x00FF0000) >> 16;
		lba_io[3] = (lba & 0xFF000000) >> 24;
		lba_io[4] = 0; // LBA28 is integer, so 32-bits are enough to access 2TB.
		lba_io[5] = 0; // LBA28 is integer, so 32-bits are enough to access 2TB.
		head = 0; // Lower 4-bits of HDDEVSEL are not used here.
	} else if (dev->features & 0x200) { // Drive supports LBA?
		// LBA28:
		lba_mode = 1;
		lba_io[0] = (lba & 0x00000FF) >> 0;
		lba_io[1] = (lba & 0x000FF00) >> 8;
		lba_io[2] = (lba & 0x0FF0000) >> 16;
		lba_io[3] = 0; // These Registers are not used here.
		lba_io[4] = 0; // These Registers are not used here.
		lba_io[5] = 0; // These Registers are not used here.
		head = (lba & 0xF000000) >> 24;
	} else {
		// CHS:
		lba_mode = 0;
		sect = (lba % 63) + 1;
		cyl = (lba + 1 - sect) / (16 * 63);
		lba_io[0] = sect;
		lba_io[1] = (cyl >> 0) & 0xFF;
		lba_io[2] = (cyl >> 8) & 0xFF;
		lba_io[3] = 0;
		lba_io[4] = 0;
		lba_io[5] = 0;
		head = (lba + 1 - sect) % (16 * 63) /
			   (63); // Head number is written to HDDEVSEL lower 4-bits.
	}

	// TODO: support DMA
	dma = 0;

	// (III) Wait if the drive is busy;
	while (ide_channel_read(chan, ATA_REG_STATUS) & ATA_SR_BUSY) {
	}

	// (IV) Select Drive from the controller;
	if (lba_mode == 0) {
		ide_channel_write(chan, ATA_REG_HDDEVSEL,
						  0xA0 | (slavebit << 4) | head); // Drive & CHS.
	} else {
		ide_channel_write(chan, ATA_REG_HDDEVSEL,
						  0xE0 | (slavebit << 4) | head); // Drive & LBA
	}

	// (V) Write Parameters;
	if (lba_mode == 2) {
		ide_channel_write(chan, ATA_REG_SECCOUNT1, 0);
		ide_channel_write(chan, ATA_REG_LBA3, lba_io[3]);
		ide_channel_write(chan, ATA_REG_LBA4, lba_io[4]);
		ide_channel_write(chan, ATA_REG_LBA5, lba_io[5]);
	}
	ide_channel_write(chan, ATA_REG_SECCOUNT0, numsects);
	ide_channel_write(chan, ATA_REG_LBA0, lba_io[0]);
	ide_channel_write(chan, ATA_REG_LBA1, lba_io[1]);
	ide_channel_write(chan, ATA_REG_LBA2, lba_io[2]);

	// (VI) Select the command and send it;
	// Routine that is followed:
	// If ( DMA & LBA48)   DO_DMA_EXT;
	// If ( DMA & LBA28)   DO_DMA_LBA;
	// If ( DMA & LBA28)   DO_DMA_CHS;
	// If (!DMA & LBA48)   DO_PIO_EXT;
	// If (!DMA & LBA28)   DO_PIO_LBA;
	// If (!DMA & !LBA#)   DO_PIO_CHS;
	if (lba_mode == 0 && dma == 0 && direction == 0)
		cmd = ATA_CMD_READ_PIO;
	if (lba_mode == 1 && dma == 0 && direction == 0)
		cmd = ATA_CMD_READ_PIO;
	if (lba_mode == 2 && dma == 0 && direction == 0)
		cmd = ATA_CMD_READ_PIO_EXT;
	if (lba_mode == 0 && dma == 1 && direction == 0)
		cmd = ATA_CMD_READ_DMA;
	if (lba_mode == 1 && dma == 1 && direction == 0)
		cmd = ATA_CMD_READ_DMA;
	if (lba_mode == 2 && dma == 1 && direction == 0)
		cmd = ATA_CMD_READ_DMA_EXT;
	if (lba_mode == 0 && dma == 0 && direction == 1)
		cmd = ATA_CMD_WRITE_PIO;
	if (lba_mode == 1 && dma == 0 && direction == 1)
		cmd = ATA_CMD_WRITE_PIO;
	if (lba_mode == 2 && dma == 0 && direction == 1)
		cmd = ATA_CMD_WRITE_PIO_EXT;
	if (lba_mode == 0 && dma == 1 && direction == 1)
		cmd = ATA_CMD_WRITE_DMA;
	if (lba_mode == 1 && dma == 1 && direction == 1)
		cmd = ATA_CMD_WRITE_DMA;
	if (lba_mode == 2 && dma == 1 && direction == 1)
		cmd = ATA_CMD_WRITE_DMA_EXT;
	ide_channel_write(chan, ATA_REG_COMMAND, cmd);

	if (dma) {
		if (direction == 0) {
			// TODO: DMA Read.
		} else {
			// TODO: DMA Write
		}
	} else {
		if (direction == 0) {
			// PIO Read.
			for (i = 0; i < numsects; i++) {
				if ((err = ide_channel_poll(chan, 1))) {
					return err; // Polling, set error and exit if there is.
				}
				__asm__ volatile("pushw %es");
				__asm__ volatile("mov %%ax, %%es" : : "a"(selector));
				// receive data
				__asm__ volatile("rep insw" : : "c"(words), "d"(bus), "D"(edi));
				__asm__ volatile("popw %es");
				edi += (words * 2);
			}
		} else {
			// PIO Write.
			for (i = 0; i < numsects; i++) {
				ide_channel_poll(chan, 0); // Polling.
				__asm__ volatile("pushw %ds");
				__asm__ volatile("mov %%ax, %%ds" ::"a"(selector));
				__asm__ volatile("rep outsw" ::"c"(words), "d"(bus),
								 "S"(edi)); // Send Data
				__asm__ volatile("popw %ds");
				edi += (words * 2);
			}
			ide_channel_write(chan, ATA_REG_COMMAND,
							  (uint8_t[]){ ATA_CMD_CACHE_FLUSH,
										   ATA_CMD_CACHE_FLUSH,
										   ATA_CMD_CACHE_FLUSH_EXT }[lba_mode]);
			ide_channel_poll(chan, 0); // Polling.
		}
	}

	return 0; // Easy, isn't it?
}

uint32_t ide_device_read_sectors(struct ide_device *dev, uint8_t numsects,
								 uint32_t lba, uint16_t es, uint32_t edi)
{
	// 1: Check if the drive presents:
	// ==================================
	if (!dev->is_reserved) {
		return 0x1; // Drive Not Found!
	}

	// 2: Check if inputs are valid:
	// ==================================
	else if (((lba + numsects) > dev->size_in_sectors) &&
			 (dev->type == IDE_ATA)) {
		return 0x2; // Seeking to invalid position.
	}

	// 3: Read in PIO Mode through Polling & IRQs:
	// ============================================
	else {
		uint8_t err = 0;
		if (dev->type == IDE_ATA) {
			err = ide_device_ata_access(dev, ATA_READ, lba, numsects, es, edi);
		} else if (dev->type == IDE_ATAPI) {
			// for (i = 0; i < numsects; i++)
			//    err = ide_atapi_read(drive, lba + i, 1, es, edi + (i*2048));
			panic("atapi unimplemented- todo");
		}
		return ide_device_print_poll_error(dev, err);
	}
}

uint32_t ide_device_write_sectors(struct ide_device *dev, uint8_t numsects,
								  uint32_t lba, uint16_t es, uint16_t edi)
{
	// 1: Check if the drive presents:
	// ==================================
	if (!dev->is_reserved) {
		return 0x1; // Drive Not Found!
	}
	// 2: Check if inputs are valid:
	// ==================================
	else if (((lba + numsects) > dev->size_in_sectors) &&
			 (dev->type == IDE_ATA)) {
		return 0x2; // Seeking to invalid position.
	}
	// 3: Read in PIO Mode through Polling & IRQs:
	// ============================================
	else {
		uint8_t err = 0;
		if (dev->type == IDE_ATA) {
			err = ide_device_ata_access(dev, ATA_WRITE, lba, numsects, es, edi);
		} else if (dev->type == IDE_ATAPI) {
			panic("atapi unimplemented- todo");
			err = POLL_ERROR_WRITE_PROTECTED;
		}
		return ide_device_print_poll_error(dev, err);
	}
}

bool ata_init(void)
{
	struct pci_device dev;

	if (!pci_findby_class(&dev, CLASS_MASS_STORAGE_CONTROLLER,
						  SUBCLASS_IDE_CONTROLLER, NULL)) {
		TRACE("No disks found by PCI class");
		return false;
	}

	// const uint8_t prog_if = pci_rcfg_b(dev, PCI_PROG_IF_B);
	const uint8_t header_type = pci_rcfg_b(dev, PCI_HEADER_TYPE_B);

	if (header_type != 0x0) {
		TRACE("Wrong header type for IDE_CONTROLLER device, not reading BARs");
		return false;
	}

	// const bool primary_channel_is_pci_native =
	// 	prog_if & IDE_PROG_IF_PRIMARY_CHANNEL_IS_PCI_NATIVE_FLAG;
	// const bool primary_channel_can_switch_native_mode =
	// 	prog_if &
	// 	IDE_PROG_IF_PRIMARY_CHANNEL_CAN_SWITCH_TO_AND_FROM_PCI_NATIVE_FLAG;
	// const bool secondary_channel_is_pci_native =
	// 	prog_if & IDE_PROG_IF_SECONDARY_CHANNEL_IS_PCI_NATIVE_FLAG;
	// const bool secondary_channel_can_switch_native_mode =
	// 	prog_if &
	// 	IDE_PROG_IF_SECONDARY_CHANNEL_CAN_SWITCH_TO_AND_FROM_PCI_NATIVE_FLAG;
	// const bool device_supports_dma = prog_if & IDE_PROG_IF_DMA_SUPPORT_FLAG;

	const uint32_t BAR0 = pci_rcfg_d(dev, PCI_BAR0_D);
	const uint32_t BAR1 = pci_rcfg_d(dev, PCI_BAR1_D);
	const uint32_t BAR2 = pci_rcfg_d(dev, PCI_BAR2_D);
	const uint32_t BAR3 = pci_rcfg_d(dev, PCI_BAR3_D);
	const uint32_t BAR4 = pci_rcfg_d(dev, PCI_BAR4_D);

	// numbers for supporting only parallel IDE
	// ide_initialize(0x1F0, 0x3F6, 0x170, 0x376, 0x000);

	ide_initialize(BAR0, BAR1, BAR2, BAR3, BAR4);

	return true;
}
