#include <lib.h>
#include <comus/mboot.h>

#include "mboot.h"

#define MULTIBOOT_TAG_TYPE_ACPI_OLD 14
#define MULTIBOOT_TAG_TYPE_ACPI_NEW 15

struct multiboot_tag_old_acpi {
	uint32_t type;
	uint32_t size;
	uint8_t rsdp[];
};

struct multiboot_tag_new_acpi {
	uint32_t type;
	uint32_t size;
	uint8_t rsdp[];
};

void *mboot_get_rsdp(void)
{
	void *tag;

	// acpi 2.0
	tag = locate_mboot_table(MULTIBOOT_TAG_TYPE_ACPI_NEW);
	if (tag != NULL) {
		struct multiboot_tag_new_acpi *rsdp = (struct multiboot_tag_new_acpi *)tag;
		return rsdp->rsdp;
	}

	// acpi 1.0
	tag = locate_mboot_table(MULTIBOOT_TAG_TYPE_ACPI_OLD);
	if (tag != NULL) {
		struct multiboot_tag_old_acpi *rsdp = (struct multiboot_tag_old_acpi *)tag;
		return rsdp->rsdp;
	}

	return NULL;
}
