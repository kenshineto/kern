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

struct ide_channel {
	uint16_t io_base;
	uint16_t control_base;
	uint16_t bus_master_ide_base;
	uint8_t no_interrupt;
};

struct ide_device {
	bool exists;
	uint8_t channel_idx; // 0 (primary channel) or 1 (secondary channel)
	uint8_t drive_idx; // 0 (master) or 1 (slave)
	uint16_t type; // 0: ATA 1:ATAPI.
	uint16_t drive_signature;
	uint16_t features;
	uint32_t supported_command_sets;
	uint32_t size_in_sectors;
	uint8_t model_str[41];
};

struct ide_channel ide_channels[2];
struct ide_device ide_devices[4];

static volatile uint8_t ide_irq_invoked = 0;
// static uint8_t atapi_packet[12] = { 0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static struct ide_device *device(const uint8_t idx)
{
	assert(idx < (sizeof(ide_devices) / sizeof(struct ide_device)),
		   "out of bounds");
	return ide_devices + idx;
}

static struct ide_channel *channel(const uint8_t idx)
{
	assert(idx < (sizeof(ide_channels) / sizeof(struct ide_channel)),
		   "out of bounds");
	return ide_channels + idx;
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
		panic("invalid ide channel register %u", reg);
	}

	if (disable_interrupts) {
		ide_channel_write(channel, ATA_REG_CONTROL, channel->no_interrupt);
	}

	return result;
}

static void ide_read_id_space_buffer(struct ide_channel *channel, uint8_t reg,
									 uint32_t *out_buffer,
									 size_t out_buffer_size_bytes)
{
	const bool disable_interrupts = reg > 0x07 && reg < 0x0C;

	const size_t quads = out_buffer_size_bytes / 4;
	assert(sizeof(uint32_t) == 4, "what.");
	assert(out_buffer_size_bytes % 4 == 0,
		   "id space buffer size not divisible by 4");

	if (disable_interrupts)
		ide_channel_write(channel, ATA_REG_CONTROL,
						  0x80 | channel->no_interrupt);

	if (reg < 0x08)
		rep_inl(channel->io_base + reg - 0x00, out_buffer, quads);
	else if (reg < 0x0C)
		rep_inl(channel->io_base + reg - 0x06, out_buffer, quads);
	else if (reg < 0x0E)
		rep_inl(channel->control_base + reg - 0x0A, out_buffer, quads);
	else if (reg < 0x16)
		rep_inl(channel->bus_master_ide_base + reg - 0x0E, out_buffer, quads);

	if (disable_interrupts)
		ide_channel_write(channel, ATA_REG_CONTROL, channel->no_interrupt);
}

static enum ide_error ide_channel_poll(struct ide_channel *channel,
									   bool advanced_check)
{
	// delay 400 nanosecond for busy to be set
	for (int i = 0; i < 4; i++) {
		// reading the alt status port wastes 100ns; loop four times
		ide_channel_read(channel, ATA_REG_ALTSTATUS);
	}

	// wait until not busy
	while (ide_channel_read(channel, ATA_REG_STATUS) & ATA_SR_BUSY)
		;

	if (advanced_check) {
		uint8_t state = ide_channel_read(channel, ATA_REG_STATUS);

        // check for errors or faults
		if (state & ATA_SR_ERROR) {
			return IDE_ERROR_POLL_STATUS_REGISTER_ERROR;
		}

		if (state & ATA_SR_DRIVEWRITEFAULT) {
			return IDE_ERROR_POLL_DEVICE_FAULT;
		}

        // then check if drive is ready
		if ((state & ATA_SR_DRIVEREQUESTREADY) == 0) {
			return IDE_ERROR_POLL_DRIVE_REQUEST_NOT_READY;
		}
	}

	return IDE_ERROR_OK;
}

static void ide_error_print(struct ide_device *dev, const enum ide_error err)
{
#if LOG_LEVEL >= LOG_LVL_ERROR
	if (err == IDE_ERROR_OK)
		return;

	kprintf("IDE ERROR:\n");
	if (err == IDE_ERROR_POLL_DEVICE_FAULT)
		kprintf("\t> poll failed, device fault");

	else if (err == IDE_ERROR_POLL_STATUS_REGISTER_ERROR) {
		uint8_t status_reg =
			ide_channel_read(channel(dev->channel_idx), ATA_REG_ERROR);
		if (status_reg & ATA_ER_NOADDRESSMARK)
			kprintf("\t> poll failed, no address mark found");
		if (status_reg & ATA_ER_TRACK0NOTFOUND)
			kprintf("\t> poll failed, no media or media error");
		if (status_reg & ATA_ER_COMMANDABORTED)
			kprintf("\t> poll failed, drive aborted command");
		if (status_reg & ATA_ER_MEDIACHANGEREQUEST)
			kprintf("\t> poll failed, no media or media error");
		if (status_reg & ATA_ER_IDMARKNOTFOUND)
			kprintf(
				"\t> poll failed, ID mark not found, unable to read ID space");
		if (status_reg & ATA_ER_MEDIACHANGED)
			kprintf("\t> poll failed, no media or media error");
		if (status_reg & ATA_ER_UNCORRECTABLE)
			kprintf("\t> poll failed, uncorrectable data error");
		if (status_reg & ATA_ER_BADBLOCK)
			kprintf("\t> poll failed, bad sectors");
	} else if (err == IDE_ERROR_POLL_DRIVE_REQUEST_NOT_READY) {
		kprintf("\t> poll read nothing, drive request not ready");
	} else if (err == IDE_ERROR_POLL_WRITE_PROTECTED) {
		kprintf("\t> unable to poll, drive is write-protected");
	}

	if (dev->channel_idx >= 2 || dev->drive_idx >= 2)
		panic("out of bounds");

	static const char *channel_displaynames[] = { "primary", "secondary" };
	static const char *device_displaynames[] = { "master", "slave" };

	const char *channel_displayname = channel_displaynames[dev->channel_idx];
	const char *device_displayname = device_displaynames[dev->drive_idx];

	kprintf("\n\t> caused by: [%s %s] %s", channel_displayname,
			device_displayname, dev->model_str);
#endif
}

static void ide_initialize(uint32_t BAR0, uint32_t BAR1, uint32_t BAR2,
						   uint32_t BAR3, uint32_t BAR4)
{
	// calculate which io ports interface with the IDE controller
	ide_channels[ATA_PRIMARY] = (struct ide_channel){
		.io_base = (BAR0 & 0xFFFFFFFC) + 0x1F0 * (!BAR0),
		.control_base = (BAR1 & 0xFFFFFFFC) + 0x3F6 * (!BAR1),
		.bus_master_ide_base = (BAR4 & 0xFFFFFFFC) + 0,
	};
	ide_channels[ATA_SECONDARY] = (struct ide_channel){
		.io_base = (BAR2 & 0xFFFFFFFC) + 0x170 * (!BAR2),
		.control_base = (BAR3 & 0xFFFFFFFC) + 0x376 * (!BAR3),
		.bus_master_ide_base = (BAR4 & 0xFFFFFFFC) + 8,
	};

    // disable irqs
	ide_channel_write(channel(ATA_PRIMARY), ATA_REG_CONTROL, 2);
	ide_channel_write(channel(ATA_SECONDARY), ATA_REG_CONTROL, 2);

    // detect disks by writing CMD_IDENTIFY to each one and checking for err.
    // if device exists, ask for its ID space and copy out info about the
    // device into the ide_device struct
	uint32_t device_count = 0;
	for (uint8_t channel_idx = 0; channel_idx < 2; channel_idx++) {
		// drive idx is like device_count but it starts at 0 per channel
		// and increments regardless of whether a device is present
		for (uint8_t drive_idx = 0; drive_idx < 2; drive_idx++) {
			// select our uninitialized device from preallocated buffer
			struct ide_device *dev = device(device_count);

			// select our initialized channel
			struct ide_channel *chan = channel(channel_idx);

			// default drive to not existing
			dev->exists = false;

			// select drive:
			ide_channel_write(chan, ATA_REG_HDDEVSEL, 0xA0 | (drive_idx << 4));
			kspin_milliseconds(1);

			// request device identification:
			ide_channel_write(chan, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
			kspin_milliseconds(1);

			if (ide_channel_read(chan, ATA_REG_STATUS) == 0) {
				continue; // if status == 0, no device
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

			// probe for ATAPI devices, though they aren't implemented
			uint8_t type = IDE_ATA;
			if (ata_err) {
				uint8_t cl = ide_channel_read(chan, ATA_REG_LBA1);
				uint8_t ch = ide_channel_read(chan, ATA_REG_LBA2);

				if (cl == 0x14 && ch == 0xEB) {
					WARN("ATAPI device found but ATAPI is not supported");
					type = IDE_ATAPI;
				} else if (cl == 0x69 && ch == 0x96) {
					WARN("ATAPI device found but ATAPI is not supported");
					type = IDE_ATAPI;
				} else {
					// unknown type (may not be a device?)
					continue;
				}

				ide_channel_write(chan, ATA_REG_COMMAND,
								  ATA_CMD_IDENTIFY_PACKET);
				// sleep(1);
				kspin_milliseconds(1);
			}

			static uint8_t id_space_buf[2048] = { 0 };
			ide_read_id_space_buffer(chan, ATA_REG_DATA,
									 (uint32_t *)id_space_buf, 512);

			// read device info from the id space buffer into our struct:
			*dev = (struct ide_device){
				.exists = true,
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
				// lba48 
				dev->size_in_sectors =
					*((uint32_t *)(id_space_buf + ATA_IDENT_MAX_LBA_EXT));
			} else {
				// lba28
				dev->size_in_sectors =
					*((uint32_t *)(id_space_buf + ATA_IDENT_MAX_LBA));
			}

			// string indicates model of device like "Western Digital HDD" etc
			for (uint8_t i = 0; i < 40; i += 2) {
				dev->model_str[i] = id_space_buf[ATA_IDENT_MODEL + i + 1];
				dev->model_str[i + 1] = id_space_buf[ATA_IDENT_MODEL + i];
			}
			dev->model_str[40] = 0; // null-terminate string

			device_count++;
		}
	}
}

enum lba_mode {
	CHS,
	LBA28,
	LBA48,
};

enum access_mode {
	READ,
	WRITE,
};

static uint8_t get_ata_cmd_for_access(enum lba_mode lba_mode,
									  enum access_mode mode)
{
    // outline of the algorithm:
	// If ( dma & lba48)   DO_DMA_EXT;
	// If ( dma & lba28)   DO_DMA_LBA;
	// If ( dma & lba28)   DO_DMA_CHS;
	// If (!dma & lba48)   DO_PIO_EXT;
	// If (!dma & lba28)   DO_PIO_LBA;
	// If (!dma & !lba#)   DO_PIO_CHS;

	if (mode == READ) {
		switch (lba_mode) {
		case CHS:
			return ATA_CMD_READ_PIO;
		case LBA28:
			return ATA_CMD_READ_PIO;
		case LBA48:
			return ATA_CMD_READ_PIO_EXT;
		}
		// if (lba_mode == 0 && dma == 1 && direction == 0)
		// 	cmd = ATA_CMD_READ_DMA;
		// if (lba_mode == 1 && dma == 1 && direction == 0)
		// 	cmd = ATA_CMD_READ_DMA;
		// if (lba_mode == 2 && dma == 1 && direction == 0)
		// 	cmd = ATA_CMD_READ_DMA_EXT;
	} else {
		assert(mode == WRITE, "unexpected access mode %d", mode);
		switch (lba_mode) {
		case CHS:
			return ATA_CMD_WRITE_PIO;
		case LBA28:
			return ATA_CMD_WRITE_PIO;
		case LBA48:
			return ATA_CMD_WRITE_PIO_EXT;
		}
		// if (lba_mode == 0 && dma == 1 && direction == 1)
		// 	cmd = ATA_CMD_WRITE_DMA;
		// if (lba_mode == 1 && dma == 1 && direction == 1)
		// 	cmd = ATA_CMD_WRITE_DMA;
		// if (lba_mode == 2 && dma == 1 && direction == 1)
		// 	cmd = ATA_CMD_WRITE_DMA_EXT;
	}
    panic("unreachable");
	return -1;
}

static enum ide_error ide_device_ata_access(struct ide_device *dev,
											enum access_mode mode, uint32_t lba,
											uint16_t numsects,
											uint16_t buf[numsects * 256])
{
	struct ide_channel *chan = channel(dev->channel_idx);
	enum lba_mode lba_mode;
	uint8_t head;
	uint8_t sect;
	uint16_t cylinder; // only used when lba_mode is CHS
	uint8_t lba_io[6];

	// disable irqs because we are using polling
	ide_channel_write(chan, ATA_REG_CONTROL,
					  chan->no_interrupt = (ide_irq_invoked = 0x0) + 0x02);

	// select one from lba28, lba48 or CHS, and fill lba_io with the parameters
	// for the disk access command
	if (lba >= 0x10000000 || numsects > UINT8_MAX) {
		// drive should support LBA in this case, or you are giving a bad LBA
		lba_mode = LBA48;
		lba_io[0] = (lba & 0x000000FF) >> 0;
		lba_io[1] = (lba & 0x0000FF00) >> 8;
		lba_io[2] = (lba & 0x00FF0000) >> 16;
		lba_io[3] = (lba & 0xFF000000) >> 24;
		lba_io[4] = 0; // LBA28 is integer, so 32-bits are enough to access 2TB
		lba_io[5] = 0; // LBA28 is integer, so 32-bits are enough to access 2TB
		head = 0; // lower 4-bits of HDDEVSEL are not used here
	} else if (dev->features & 0x200) { // drive supports LBA?
		lba_mode = LBA28;
		lba_io[0] = (lba & 0x00000FF) >> 0;
		lba_io[1] = (lba & 0x000FF00) >> 8;
		lba_io[2] = (lba & 0x0FF0000) >> 16;
		lba_io[3] = 0; // these registers are not used here
		lba_io[4] = 0; // these registers are not used here
		lba_io[5] = 0; // these registers are not used here
		head = (lba & 0xF000000) >> 24;
	} else {
		lba_mode = CHS;
		sect = (lba % 63) + 1;
		cylinder = (lba + 1 - sect) / (16 * 63);
		lba_io[0] = sect;
		lba_io[1] = (cylinder >> 0) & 0xFF;
		lba_io[2] = (cylinder >> 8) & 0xFF;
		lba_io[3] = 0;
		lba_io[4] = 0;
		lba_io[5] = 0;
		// head number is written to HDDEVSEL lower 4-bits
		head = (lba + 1 - sect) % (16 * 63) / (63);
	}

	// wait if the drive is busy
	while (ide_channel_read(chan, ATA_REG_STATUS) & ATA_SR_BUSY)
		;

	if (dev->drive_idx > 1)
		panic("unexpected drive_idx");

	// select our drive
	if (lba_mode == CHS) {
		ide_channel_write(chan, ATA_REG_HDDEVSEL,
						  0xA0 | (dev->drive_idx << 4) | head);
	} else if (lba_mode == LBA28) {
		ide_channel_write(chan, ATA_REG_HDDEVSEL,
						  0xE0 | (dev->drive_idx << 4) | head);
	} else {
		ide_channel_write(chan, ATA_REG_HDDEVSEL, 0x40 | (dev->drive_idx << 4));
	}

	// actually write the parameters
	if (lba_mode == LBA48) {
		ide_channel_write(chan, ATA_REG_SECCOUNT1, (numsects >> 8) & 0xff);
		ide_channel_write(chan, ATA_REG_LBA3, lba_io[3]);
		ide_channel_write(chan, ATA_REG_LBA4, lba_io[4]);
		ide_channel_write(chan, ATA_REG_LBA5, lba_io[5]);
	}
	ide_channel_write(chan, ATA_REG_SECCOUNT0, numsects & 0xff);
	ide_channel_write(chan, ATA_REG_LBA0, lba_io[0]);
	ide_channel_write(chan, ATA_REG_LBA1, lba_io[1]);
	ide_channel_write(chan, ATA_REG_LBA2, lba_io[2]);

	ide_channel_write(chan, ATA_REG_COMMAND,
					  get_ata_cmd_for_access(lba_mode, mode));

	// TODO: if (dma) { ... } else {
	if (mode == READ) {
		// just read all the bytes of the sectors out of the io port
		for (size_t i = 0; i < numsects; i++) {
			enum ide_error ret = ide_channel_poll(chan, 1);
			if (ret)
				return ret;

			// receive data
			rep_inw(chan->io_base, &buf[i * 256], 256);
		}
	} else {
		// just write all the bytes of the sectors into the io port
		for (size_t i = 0; i < numsects; i++) {
			enum ide_error err = ide_channel_poll(chan, 0);
#if LOG_LEVEL >= LOG_LVL_WARN
			if (err) {
				WARN("DRIVE WRITE FAILED:");
				ide_error_print(dev, err);
			}
#endif
			rep_outw(chan->io_base, &buf[i * 256], 256);
		}

		// flush the cache to fully complete the write
		ide_channel_write(chan, ATA_REG_COMMAND,
						  (uint8_t[]){ ATA_CMD_CACHE_FLUSH, ATA_CMD_CACHE_FLUSH,
									   ATA_CMD_CACHE_FLUSH_EXT }[lba_mode]);
		enum ide_error err = ide_channel_poll(chan, 0);
#if LOG_LEVEL >= LOG_LVL_WARN
		if (err) {
			WARN("DRIVE WRITE FAILED, CACHE FLUSH ERR:");
			ide_error_print(dev, err);
		}
#endif
	}

	return IDE_ERROR_OK;
}

enum ide_error ide_device_read_sectors(ide_device_t dev_identifier,
									   uint16_t numsects, uint32_t lba,
									   uint16_t buf[numsects * 256])
{
	struct ide_device *dev = device(dev_identifier);

	// check if the drive present
	if (!dev->exists)
		return IDE_ERROR_NULL_DEVICE;

	// check if inputs are valid
	if (((lba + numsects) > dev->size_in_sectors) && (dev->type == IDE_ATA))
		return 1; // seeking to invalid position.

	// read in PIO Mode through polling & irqs
	enum ide_error err = IDE_ERROR_OK;
	if (dev->type == IDE_ATA) {
		err = ide_device_ata_access(dev, ATA_READ, lba, numsects, buf);
	} else if (dev->type == IDE_ATAPI) {
		// for (i = 0; i < numsects; i++)
		//    err = ide_atapi_read(drive, lba + i, 1, es, edi + (i*2048));
		//panic("atapi unimplemented- todo");

        // soft error instead of panic
		return IDE_ERROR_UNIMPLEMENTED;
	}

	if (err) {
		ERROR("DRIVE READ FAILED:");
		ide_error_print(dev, err);
	}

	return err;
}

enum ide_error ide_device_write_sectors(ide_device_t device_identifier,
										uint16_t numsects, uint32_t lba,
										uint16_t buf[numsects * 256])
{
	struct ide_device *dev = device(device_identifier);

	// check if the drive present
	if (!dev->exists)
		return IDE_ERROR_NULL_DEVICE;

	// check if inputs are valid
	if (((lba + numsects) > dev->size_in_sectors) && (dev->type == IDE_ATA))
		return 1; // seeking to invalid position.

	// read in PIO Mode through polling & irqs
	enum ide_error err = IDE_ERROR_OK;
	if (dev->type == IDE_ATA) {
		err = ide_device_ata_access(dev, ATA_WRITE, lba, numsects, buf);
	} else if (dev->type == IDE_ATAPI) {
		//panic("atapi unimplemented- todo");
		//err = IDE_ERROR_POLL_WRITE_PROTECTED;
		return 1;
	}

	if (err)
		ide_error_print(dev, err);

	return err;
}

enum ide_error ata_init(void)
{
	struct pci_device dev;

	if (!pci_findby_class(&dev, CLASS_MASS_STORAGE_CONTROLLER,
						  SUBCLASS_IDE_CONTROLLER, NULL)) {
		TRACE("No disks found by PCI class");
		return IDE_ERROR_INIT_NO_IDE_CONTROLLER;
	}

	// const uint8_t prog_if = pci_rcfg_b(dev, PCI_PROG_IF_B);
	const uint8_t header_type = pci_rcfg_b(dev, PCI_HEADER_TYPE_B);

	if (header_type != 0x0) {
		TRACE("Wrong header type for IDE_CONTROLLER device, not reading BARs");
		return IDE_ERROR_INIT_BAD_HEADER;
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

	return IDE_ERROR_OK;
}

struct ide_devicelist ide_devices_enumerate(void)
{
	struct ide_devicelist out = {
		.num_devices = 0,
	};

	for (size_t i = 0; i < 4; ++i) {
		struct ide_device *dev = device(i);
		if (dev->exists) {
			out.devices[out.num_devices] = i;
			out.num_devices += 1;
		}
	}

	return out;
}

void ata_report(void)
{
	if (!ide_devices[0].exists)
		return;

	kprintf("ATA DEVICES\n");
	for (size_t i = 0; i < 4; i++) {
		struct ide_device *dev = &ide_devices[i];
		if (!dev->exists)
			continue;
		char size[20];
		btoa(dev->size_in_sectors * 512, size);
		kprintf(
			"[%u:%u] %s\nType: %s\nSignature: %#04x\nFeatures: %#04x\nCommands: %#08x\nSize: %s\n\n",
			dev->channel_idx, dev->drive_idx, dev->model_str,
			dev->type ? "ATAPI" : "ATA", dev->drive_signature, dev->features,
			dev->supported_command_sets, size);
	}
}
