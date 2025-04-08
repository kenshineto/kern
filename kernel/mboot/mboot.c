#include <lib.h>
#include <comus/mboot.h>

#include "mboot.h"

static volatile void *mboot;

void mboot_init(long magic, volatile void *ptr)
{
	if (magic != MULTIBOOT2_BOOTLOADER_MAGIC)
		panic("invalid multiboot magic");
	mboot = ptr;
}

void *locate_mboot_table(uint32_t type)
{
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
