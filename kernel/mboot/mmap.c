#include <lib.h>
#include <comus/mboot.h>

#include "mboot.h"

#define MULTIBOOT_TAG_TYPE_MMAP 6

struct multiboot_mmap_entry {
	uint64_t addr;
	uint64_t len;
	uint32_t type;
	uint32_t zero;
};

struct multiboot_tag_mmap {
	uint32_t type;
	uint32_t size;
	uint32_t entry_size;
	uint32_t entry_version;
	struct multiboot_mmap_entry entries[];
};

int mboot_get_mmap(struct memory_map *res)
{
	void *tag = locate_mboot_table(MULTIBOOT_TAG_TYPE_MMAP);
	if (tag == NULL)
		return 1;

	struct multiboot_tag_mmap *mmap = (struct multiboot_tag_mmap *)tag;

	int idx = 0;
	uintptr_t i = (uintptr_t)mmap->entries;
	for (; i < (uintptr_t)mmap->entries + mmap->size;
		 i += mmap->entry_size, idx++) {
		struct multiboot_mmap_entry *seg = (struct multiboot_mmap_entry *)i;
		res->entries[idx].addr = seg->addr;
		res->entries[idx].len = seg->len;
		res->entries[idx].type = seg->type;
	}
	res->entry_count = idx;

	return 0;
}
