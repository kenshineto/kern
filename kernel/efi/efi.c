#include <lib.h>
#include <comus/efi.h>
#include <comus/mboot.h>
#include <comus/memory.h>
#include <efi.h>

#include "efi.h"

void efi_init(EFI_HANDLE IH, EFI_SYSTEM_TABLE *ST)
{
	EFI_STATUS status;

	if (IH == NULL || ST == NULL)
		return;

	status = efi_load_mmap(ST);
	if (EFI_ERROR(status))
		panic("Failed to load efi memory map, EFI_STATUS = %lu\n", status);

	status = efi_load_gop(ST);
	if (EFI_ERROR(status))
		panic(
			"Failed to locate graphics output protocol (GOP), EFI_STATUS = %lu\n",
			status);

	ST->BootServices->ExitBootServices(IH, 0);
}
