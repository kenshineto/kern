#include <lib.h>
#include <comus/mboot.h>

#include "mboot.h"

static const char *segment_type[] = { "Reserved",	 "Free",
									  "Reserved",	 "ACPI Reserved",
									  "Hibernation", "Defective",
									  "Unknown" };

int mboot_get_mmap(struct memory_map *res)
{
	void *tag = locate_mboot_table(MBOOT_MEMORY_MAP);
	if (tag == NULL)
		return 1;

	struct mboot_tag_mmap *mmap = (struct mboot_tag_mmap *)tag;

	int idx = 0;
	uintptr_t i = (uintptr_t)mmap->entries;
	kprintf("MEMORY MAP\n");
	char buf[20];
	for (; i < (uintptr_t)mmap->entries + mmap->size;
		 i += mmap->entry_size, idx++) {
		struct mboot_mmap_entry *seg = (struct mboot_mmap_entry *)i;
		const char *type = NULL;
		if (seg->type > 6)
			type = segment_type[6];
		else
			type = segment_type[seg->type];
		kprintf("ADDR: %16p  LEN: %4s  TYPE: %s (%d)\n", (void *)seg->addr,
				btoa(seg->len, buf), type, seg->type);
		if (seg->type != 1 || seg->len < 1)
			continue;
		res->entries[idx].addr = seg->addr;
		res->entries[idx].len = seg->len;
	}
	res->entry_count = idx;

	return 0;
}
