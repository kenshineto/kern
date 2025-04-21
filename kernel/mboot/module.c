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

void *mboot_get_initrd(size_t *len)
{
	void *tag = locate_mboot_table(MULTIBOOT_TAG_TYPE_MODULE);
	if (tag == NULL)
		return NULL;

	struct multiboot_tag_module *mod = (struct multiboot_tag_module *)tag;
	*len = mod->mod_end - mod->mod_start;
	return (void*) (uintptr_t) mod->mod_start;
}
