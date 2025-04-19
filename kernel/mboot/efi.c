#include <comus/mboot.h>

#include "mboot.h"

#define MULTIBOOT_TAG_TYPE_EFI64 12
#define MULTIBOOT_TAG_TYPE_EFI64_IH 20

struct multiboot_tag_efi64 {
	uint32_t type;
	uint32_t size;
	uint64_t pointer;
};

struct multiboot_tag_efi64_ih {
	uint32_t type;
	uint32_t size;
	uint64_t pointer;
};

EFI_SYSTEM_TABLE *mboot_get_efi_st(void)
{
	void *tag = locate_mboot_table(MULTIBOOT_TAG_TYPE_EFI64);
	if (tag == NULL)
		return NULL;

	struct multiboot_tag_efi64 *efi = (struct multiboot_tag_efi64 *)tag;
	return (void *)efi->pointer;
}

EFI_HANDLE mboot_get_efi_hdl(void)
{
	void *tag = locate_mboot_table(MULTIBOOT_TAG_TYPE_EFI64_IH);
	if (tag == NULL)
		return NULL;

	struct multiboot_tag_efi64_ih *ih = (struct multiboot_tag_efi64_ih *)tag;
	return (void *)ih->pointer;
}
