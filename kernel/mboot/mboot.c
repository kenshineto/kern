#include <lib.h>
#include <comus/mboot.h>

#include "mboot.h"

static volatile void *mboot = NULL;

extern char kernel_end[];

void mboot_init(long magic, volatile void *ptr)
{
	if (magic != MULTIBOOT2_BOOTLOADER_MAGIC)
		panic("invalid multiboot magic: %#08lx", magic);
	mboot = ptr;
}

void *mboot_end(void)
{
	if (mboot == NULL)
		return NULL;

	struct multiboot *info = (struct multiboot *)mboot;
	uintptr_t mboot_end, initrd_end;
	size_t initrd_len;

	mboot_end = (uintptr_t)info + info->total_size;
	initrd_end = (uintptr_t)mboot_get_initrd_phys(&initrd_len);
	if (initrd_end)
		initrd_end += initrd_len;

	return (void *)MAX(mboot_end, initrd_end);
}

void *locate_mboot_table(uint32_t type)
{
	if (mboot == NULL)
		return NULL;

	struct multiboot *info = (struct multiboot *)mboot;

	const char *mboot_end = ((char *)info) + info->total_size;
	char *tag_ptr = info->tags;

	while (tag_ptr < mboot_end) {
		struct multiboot_tag *tag = (struct multiboot_tag *)tag_ptr;

		if (tag->type == type)
			return tag;

		// goto next
		int size = tag->size;
		if (size % 8 != 0) {
			size += 8 - (size % 8);
		}
		tag_ptr += size;
	}

	return NULL;
}
