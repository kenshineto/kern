#include <comus/drivers/pci.h>
#include <comus/asm.h>
#include <lib.h>

#define CONF_ADDR 0xCF8
#define CONF_DATA 0xCFC
#define TABLE_LEN 16

struct pci_table_entry {
	struct pci_device device;
	uint16_t device_id;
	uint16_t vendor_id;
	uint8_t class;
	uint8_t subclass;
	uint8_t prog_if;
	uint8_t revision;
};

static struct pci_table_entry pci_table[TABLE_LEN];
static size_t pci_table_next = 0;

uint32_t pci_rcfg_d(struct pci_device dev, uint8_t offset)
{
	uint32_t addr = 0x80000000;
	addr |= ((uint32_t)dev.bus) << 16;
	addr |= ((uint32_t)dev.device) << 11;
	addr |= ((uint32_t)dev.function) << 8;
	addr |= offset & 0xFC;

	outl(CONF_ADDR, addr);
	uint32_t in = inl(CONF_DATA);
	return in;
}

static void print_device(struct pci_table_entry *entry)
{
	kprintf(
		"BUS: %#-4x  DEV: %#-4x  FUNC: %#-4x  ID: %04x:%04x  CLASS: %02x:%02x:%02x  REV: %#02x\n",
		entry->device.bus, entry->device.device, entry->device.function,
		entry->vendor_id, entry->device_id, entry->class, entry->subclass,
		entry->prog_if, entry->revision);
}

static struct pci_table_entry *load_device(struct pci_device dev)
{
	if (pci_table_next >= TABLE_LEN)
		panic("Too many PCI devices: limit is %d", TABLE_LEN);
	struct pci_table_entry *entry = &pci_table[pci_table_next++];
	entry->device = dev;
	uint32_t dword0 = pci_rcfg_d(dev, 0);
	uint32_t dword2 = pci_rcfg_d(dev, 8);

	entry->device_id = (dword0 >> 16) & 0xFFFF;
	entry->vendor_id = dword0 & 0xFFFF;

	entry->class = (dword2 >> 24) & 0xFF;
	entry->subclass = (dword2 >> 16) & 0xFF;
	entry->prog_if = (dword2 >> 8) & 0xFF;
	entry->revision = dword2 & 0xFF;

	return entry;
}

uint16_t pci_rcfg_w(struct pci_device dev, uint8_t offset)
{
	uint32_t dword = pci_rcfg_d(dev, offset);
	return (uint16_t)((dword >> ((offset & 2) * 8)) & 0xFFFF);
}

uint8_t pci_rcfg_b(struct pci_device dev, uint8_t offset)
{
	uint32_t dword = pci_rcfg_d(dev, offset);
	return (uint8_t)((dword >> ((offset & 3) * 8)) & 0xFF);
}

void pci_wcfg_d(struct pci_device dev, uint8_t offset, uint32_t dword)
{
	uint32_t addr = 0x80000000;
	addr |= ((uint32_t)dev.bus) << 16;
	addr |= ((uint32_t)dev.device) << 11;
	addr |= ((uint32_t)dev.function) << 8;
	addr |= offset & 0xFC;

	outl(CONF_ADDR, addr);
	outl(CONF_DATA, dword);
}

void pci_wcfg_w(struct pci_device dev, uint8_t offset, uint16_t word)
{
	size_t shift = (offset & 2) * 8;
	uint32_t dword = pci_rcfg_d(dev, offset);
	dword &= ~(0xFFFF << shift);
	dword |= word << shift;
	pci_wcfg_d(dev, offset, dword);
}

void pci_wcfg_b(struct pci_device dev, uint8_t offset, uint8_t byte)
{
	size_t shift = (offset & 3) * 8;
	uint32_t dword = pci_rcfg_d(dev, offset);
	dword &= ~(0xFF << shift);
	dword |= byte << shift;
	pci_wcfg_d(dev, offset, dword);
}

void pci_init(void)
{
	pci_table_next = 0;
	struct pci_device pcidev;
	for (int bus = 0; bus < 256; bus++) {
		pcidev.bus = bus;
		for (int dev = 0; dev < 32; dev++) {
			pcidev.device = dev;
			pcidev.function = 0;

			uint16_t vendor = pci_rcfg_w(pcidev, 0);
			if (vendor == 0xFFFF)
				continue;

			load_device(pcidev);

			uint8_t header_type = pci_rcfg_b(pcidev, 14);

			if (!(header_type & 0x80))
				continue;
			for (int func = 1; func < 8; func++) {
				pcidev.function = func;

				uint16_t vendor = pci_rcfg_w(pcidev, 0);
				if (vendor == 0xFFFF)
					continue;

				load_device(pcidev);
			}
		}
	}
}

void pci_report(void)
{
	kprintf("PCI DEVICES\n");
	for (size_t i = 0; i < pci_table_next; i++) {
		print_device(&pci_table[i]);
	}
	kprintf("\n");
}

bool pci_findby_class(struct pci_device *dest, uint8_t class, uint8_t subclass,
					  size_t *offset)
{
	size_t o = 0;
	if (offset == NULL)
		offset = &o;
	for (; *offset < pci_table_next; (*offset)++) {
		struct pci_table_entry *entry = &pci_table[*offset];
		if (entry->class == class && entry->subclass == subclass) {
			*dest = entry->device;
			return true;
		}
	}
	return false;
}

bool pci_findby_id(struct pci_device *dest, uint16_t device, uint16_t vendor,
				   size_t *offset)
{
	size_t o = 0;
	if (offset == NULL)
		offset = &o;
	for (; *offset < pci_table_next; (*offset)++) {
		struct pci_table_entry *entry = &pci_table[*offset];
		if (entry->device_id == device && entry->vendor_id == vendor) {
			*dest = entry->device;
			return true;
		}
	}
	return false;
}
