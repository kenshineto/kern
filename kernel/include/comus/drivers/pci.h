/**
 * @file pci.h
 *
 * @author Freya Murphy <freya@freyacaat.org>
 * @author Tristan Miller <trimill@trimillxyz.org>
 *
 * PCI driver
 */

#ifndef PCI_H_
#define PCI_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// common
#define PCI_VENDOR_W 0x00
#define PCI_DEVICE_W 0x02
#define PCI_COMMAND_W 0x04
#define PCI_STATUS_W 0x06
#define PCI_REVISION_B 0x08
#define PCI_PROG_IF_B 0x09
#define PCI_SUBCLASS_B 0x0A
#define PCI_CLASS_B 0x0B
#define PCI_CACHE_SIZE_B 0x0C
#define PCI_LATENCY_TIMER_B 0x0D
#define PCI_HEADER_TYPE_B 0x0E
#define PCI_BIST_B 0x0F

// header type 0
#define PCI_BAR0_D 0x10
#define PCI_BAR1_D 0x14
#define PCI_BAR2_D 0x18
#define PCI_BAR3_D 0x1C
#define PCI_BAR4_D 0x20
#define PCI_BAR5_D 0x24
#define PCI_CARDBUS_CIS_D 0x28
#define PCI_SUBSYSTEM_VENDOR_W 0x2C
#define PCI_SUBSYSTEM_W 0x2E
#define PCI_EXPANSION_ROM_D 0x30
#define PCI_CAP_PTR_B 0x34
#define PCI_INT_LINE_B 0x3C
#define PCI_INT_PIN_B 0x3D
#define PCI_MIN_GRANT_B 0x3E
#define PCI_MAX_LATENCY_B 0x3F

struct pci_device {
	uint8_t bus : 8;
	uint8_t device : 5;
	uint8_t function : 3;
};

/**
 * Load the PCI driver
 */
void pci_init(void);

bool pci_findby_class(struct pci_device *dest, uint8_t class, uint8_t subclass,
					  size_t *offset);
bool pci_findby_id(struct pci_device *dest, uint16_t device, uint16_t vendor,
				   size_t *offset);

uint32_t pci_rcfg_d(struct pci_device dev, uint8_t offset);
uint16_t pci_rcfg_w(struct pci_device dev, uint8_t offset);
uint8_t pci_rcfg_b(struct pci_device dev, uint8_t offset);

void pci_wcfg_d(struct pci_device dev, uint8_t offset, uint32_t dword);
void pci_wcfg_w(struct pci_device dev, uint8_t offset, uint16_t word);
void pci_wcfg_b(struct pci_device dev, uint8_t offset, uint8_t byte);

#endif /* pci.h */
