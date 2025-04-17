#include <lib.h>
#include <comus/asm.h>
#include <comus/memory.h>
#include <comus/drivers/pci.h>
#include <comus/drivers/gpu/bochs.h>

#define INDEX           0x1CE
#define DATA            0x1CF

#define INDEX_ID            0
#define INDEX_XRES          1
#define INDEX_YRES          2
#define INDEX_BPP           3
#define INDEX_ENABLE        4
#define INDEX_BANK          5
#define INDEX_VIRT_WIDTH    6
#define INDEX_VIRT_HEIGHT   7
#define INDEX_X_OFFSET      8
#define INDEX_Y_OFFSET      9

#define DATA_DISP_DISABLE   0x00
#define DATA_DISP_ENABLE    0x01
#define DATA_LFB_ENABLE     0x40
#define DATA_NO_CLEAR_MEM   0x80

#define BOCHS_PCI_VENDOR    0x1234
#define BOCHS_PCI_DEVICE    0x1111

#define BOCHS_WIDTH 1024
#define BOCHS_HEIGHT 768
#define BOCHS_BIT_DEPTH 32

static volatile uint32_t *framebuffer;
static uint16_t width = BOCHS_WIDTH;
static uint16_t height = BOCHS_HEIGHT;
static uint8_t bit_depth = BOCHS_BIT_DEPTH;

static void write(uint16_t index, uint16_t data) {
	outw(INDEX, index);
	outw(DATA, data);
}

static uint16_t read(uint16_t value) {
	outw(INDEX, value);
	return inw(DATA);
}

static int is_available(void) {
	return (read(INDEX_ID) == 0xB0C5);
}

static void set_mode(uint16_t width, uint16_t height, uint16_t bit_depth, int lfb, int clear) {
	write(INDEX_ENABLE, DATA_DISP_DISABLE);
	write(INDEX_XRES, width);
	write(INDEX_YRES, height);
	write(INDEX_BPP, bit_depth);
	write(INDEX_ENABLE, DATA_DISP_ENABLE |
			(lfb ? DATA_LFB_ENABLE : 0) |
			(clear ? 0 : DATA_NO_CLEAR_MEM));
}

int bochs_init(void) {
	struct pci_device bochs = {0};
	bool found = pci_findby_id(&bochs, BOCHS_PCI_DEVICE, BOCHS_PCI_VENDOR, NULL);
	if (!found)
		return 1;

	set_mode(width, height, bit_depth, true, true);
	if (!is_available())
		return 1;

	uint32_t bar0 = pci_rcfg_d(bochs, PCI_BAR0_D);
	uint32_t *addr = (uint32_t *) (uintptr_t) bar0;
	framebuffer = kmapaddr(addr, NULL, width * height * bit_depth, F_WRITEABLE);

	return 0;
}

uint32_t bochs_width(void)
{
	return width;
}

uint32_t bochs_height(void)
{
	return height;
}

uint8_t bochs_bit_depth(void)
{
	return bit_depth;
}

void bochs_set_pixel(uint32_t x, uint32_t y, uint32_t r, uint32_t g, uint32_t b)
{
	uint32_t index = x + y * width;
	framebuffer[index] = (b << 0) | (g << 8) | (r << 16);
}
