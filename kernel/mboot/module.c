#include <comus/memory.h>
#include <comus/mboot.h>

#include "mboot.h"

#define MULTIBOOT_TAG_TYPE_MODULE 3

struct multiboot_tag_module {
	uint32_t type;
	uint32_t size;
	uint32_t mod_start;
	uint32_t mod_end;
	char cmdline[];
};

static void *mapped_addr = NULL;
size_t initrd_len;

void *mboot_get_initrd_phys(size_t *len)
{
	struct multiboot_tag_module *mod;
	void *tag, *phys;

	// if already loaded, return
	if (mapped_addr) {
		*len = initrd_len;
		return mapped_addr;
	}

	// locate
	tag = locate_mboot_table(MULTIBOOT_TAG_TYPE_MODULE);
	if (tag == NULL)
		return NULL;

	mod = (struct multiboot_tag_module *)tag;
	phys = (void *)(uintptr_t)mod->mod_start;
	initrd_len = mod->mod_end - mod->mod_start;

	*len = initrd_len;
	return phys;
}

void *mboot_get_initrd(size_t *len)
{
	// get phys
	void *phys = mboot_get_initrd_phys(len);
	if (phys == NULL)
		return NULL;

	// map addr
	mapped_addr = kmapaddr(phys, NULL, initrd_len, F_PRESENT | F_WRITEABLE);
	if (mapped_addr == NULL)
		return NULL;

	return mapped_addr;
}
