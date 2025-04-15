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

#include "comus/drivers/pci.h"
#include "comus/drivers/ata.h"

bool ata_find_primary_drive(struct pci_device *out)
{
	if (!pci_findby_class(out, CLASS_MASS_STORAGE_CONTROLLER,
							SUBCLASS_IDE_CONTROLLER, NULL)) {
        return false;
    }

    const uint8_t prog_if = pci_rcfg_b(*out, PCI_PROG_IF_B);

    return true;
}


