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

// ATA statuses, read off of the command/status port
// clang-format off
#define ATA_SR_BUSY                 0x80
#define ATA_SR_DRIVEREADY           0x40
#define ATA_SR_DRIVEWRITEFAULT      0x20
#define ATA_SR_DRIVESEEKCOMPLETE    0x10
#define ATA_SR_DRIVEREQUESTREADY    0x08
#define ATA_SR_CORRECTEDATA         0x04
#define ATA_SR_INDEX                0x02
#define ATA_SR_ERROR                0x01
// clang-format on

// ATA errors, ready off of the features/error port
// clang-format off
#define ATA_ER_BADBLOCK             0x80
#define ATA_ER_UNCORRECTABLE        0x40
#define ATA_ER_MEDIACHANGED         0x20
#define ATA_ER_IDMARKNOTFOUND       0x10
#define ATA_ER_MEDIACHANGEREQUEST   0x08
#define ATA_ER_COMMANDABORTED       0x04
#define ATA_ER_TRACK0NOTFOUND       0x02
#define ATA_ER_NOADDRESSMARK        0x01
// clang-format on

// ATA commands
// clang-format off
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
// clang-format on

#include <comus/drivers/ata.h>
#include <comus/drivers/pci.h>
#include <lib.h>

bool ata_find_primary_drive(struct pci_device *out)
{
	if (!pci_findby_class(out, CLASS_MASS_STORAGE_CONTROLLER,
						  SUBCLASS_IDE_CONTROLLER, NULL)) {
		return false;
	}

	const uint8_t prog_if = pci_rcfg_b(*out, PCI_PROG_IF_B);
	const uint8_t header_type = pci_rcfg_b(*out, PCI_HEADER_TYPE_B);

	if (header_type != 0x0) {
		TRACE("Wrong header type for IDE_CONTROLLER device, not reading BARs");
		return false;
	}

	return true;
}
