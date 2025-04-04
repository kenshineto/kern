
#include "mboot.h"

void *locate_mboot_table(volatile void *mboot, uint32_t type)
{
	struct mboot_info *info = (struct mboot_info *)mboot;
	const char *mboot_end = ((char *)info) + info->total_size;

	char *tag_ptr = info->tags;

	while (tag_ptr < mboot_end) {
		struct mboot_tag *tag = (struct mboot_tag *)tag_ptr;

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
