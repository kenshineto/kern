#include <lib.h>
#include <comus/asm.h>
#include <comus/memory.h>
#include <comus/drivers/pci.h>
#include <comus/drivers/gpu.h>
#include <comus/drivers/gpu/bochs.h>

#define INDEX 0x1CE
#define DATA 0x1CF

#define INDEX_ID 0
#define INDEX_XRES 1
#define INDEX_YRES 2
#define INDEX_BPP 3
#define INDEX_ENABLE 4
#define INDEX_BANK 5
#define INDEX_VIRT_WIDTH 6
#define INDEX_VIRT_HEIGHT 7
#define INDEX_X_OFFSET 8
#define INDEX_Y_OFFSET 9

#define DATA_DISP_DISABLE 0x00
#define DATA_DISP_ENABLE 0x01
#define DATA_LFB_ENABLE 0x40
#define DATA_NO_CLEAR_MEM 0x80

#define BOCHS_PCI_VENDOR 0x1234
#define BOCHS_PCI_DEVICE 0x1111

#define BOCHS_WIDTH 1024
#define BOCHS_HEIGHT 768
#define BOCHS_BIT_DEPTH 32

struct gpu_dev bochs_dev = { 0 };

static void write(uint16_t index, uint16_t data)
{
	outw(INDEX, index);
	outw(DATA, data);
}

static uint16_t read(uint16_t value)
{
	outw(INDEX, value);
	return inw(DATA);
}

int bochs_init(struct gpu_dev **gpu_dev)
{
	struct pci_device bochs = { 0 };
	bool found =
		pci_findby_id(&bochs, BOCHS_PCI_DEVICE, BOCHS_PCI_VENDOR, NULL);
	if (!found)
		return 1;

	write(INDEX_ENABLE, DATA_DISP_DISABLE);
	write(INDEX_XRES, BOCHS_WIDTH);
	write(INDEX_YRES, BOCHS_HEIGHT);
	write(INDEX_BPP, BOCHS_BIT_DEPTH);
	write(INDEX_ENABLE, DATA_DISP_ENABLE | DATA_LFB_ENABLE);
	if (read(INDEX_ID) != 0xB0C5)
		return 1;

	uint32_t bar0 = pci_rcfg_d(bochs, PCI_BAR0_D);
	uint32_t *addr = (uint32_t *)(uintptr_t)bar0;

	bochs_dev.name = "Bochs";
	bochs_dev.width = BOCHS_WIDTH;
	bochs_dev.height = BOCHS_HEIGHT;
	bochs_dev.bit_depth = BOCHS_BIT_DEPTH;
	bochs_dev.framebuffer =
		kmapaddr(addr, NULL, BOCHS_WIDTH * BOCHS_HEIGHT * (BOCHS_BIT_DEPTH / 8),
				 F_WRITEABLE);
	*gpu_dev = &bochs_dev;

	return 0;
}
