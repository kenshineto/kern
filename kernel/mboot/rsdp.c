#include <lib.h>
#include <comus/mboot.h>

#include "mboot.h"

void *mboot_get_rsdp(void)
{
	void *tag;

	// acpi 2.0
	tag = locate_mboot_table(MBOOT_NEW_RSDP);
	if (tag != NULL) {
		struct mboot_tag_new_rsdp *rsdp = (struct mboot_tag_new_rsdp *)tag;
		return rsdp->rsdp;
	}

	// acpi 1.0
	tag = locate_mboot_table(MBOOT_OLD_RSDP);
	if (tag != NULL) {
		struct mboot_tag_old_rsdp *rsdp = (struct mboot_tag_old_rsdp *)tag;
		return rsdp->rsdp;
	}

	return NULL;
}
