#include <lib.h>
#include <efi.h>
#include <comus/efi.h>
#include <comus/memory.h>

struct memory_map efi_mmap = { 0 };

EFI_STATUS efi_load_mmap(EFI_SYSTEM_TABLE *ST)
{
	EFI_STATUS status = EFI_SUCCESS;

	EFI_MEMORY_DESCRIPTOR *memory_map = NULL;
	UINT32 version = 0;
	UINTN map_key = 0;
	UINTN descriptor_size = 0;
	UINTN memory_map_size = 0;

	status = ST->BootServices->GetMemoryMap(
		&memory_map_size, memory_map, &map_key, &descriptor_size, &version);

	assert(status != EFI_SUCCESS,
		   "GetMemoryMap should NEVER succeed the first time");
	if (status != EFI_BUFFER_TOO_SMALL)
		return status;

	UINTN encompassing_size = memory_map_size + (2 * descriptor_size);
	void *buffer = NULL;
	status = ST->BootServices->AllocatePool(EfiLoaderData, encompassing_size,
											&buffer);

	if (EFI_ERROR(status))
		return status;

	memory_map = (EFI_MEMORY_DESCRIPTOR *)buffer;
	memory_map_size = encompassing_size;
	status = ST->BootServices->GetMemoryMap(
		&memory_map_size, memory_map, &map_key, &descriptor_size, &version);

	if (EFI_ERROR(status))
		return status;

	uint32_t idx = 0;
	for (size_t i = 0; i < (memory_map_size / descriptor_size); i++) {
		EFI_MEMORY_DESCRIPTOR *seg =
			(EFI_MEMORY_DESCRIPTOR *)((char *)memory_map +
									  (descriptor_size * i));
		if (idx >= N_MMAP_ENTRY)
			panic("Too many mmap entries: limit is %d", N_MMAP_ENTRY);
		efi_mmap.entries[idx].addr = seg->PhysicalStart;
		efi_mmap.entries[idx].len = seg->NumberOfPages * PAGE_SIZE;
		switch (seg->Type) {
		case EfiReservedMemoryType:
		case EfiMemoryMappedIO:
		case EfiMemoryMappedIOPortSpace:
			efi_mmap.entries[idx].type = SEG_TYPE_RESERVED;
			break;
		case EfiConventionalMemory:
			efi_mmap.entries[idx].type = SEG_TYPE_FREE;
			break;
		case EfiACPIReclaimMemory:
		case EfiACPIMemoryNVS:
			efi_mmap.entries[idx].type = SEG_TYPE_ACPI;
			break;
		case EfiUnusableMemory:
			efi_mmap.entries[idx].type = SEG_TYPE_DEFECTIVE;
			break;
		default:
			efi_mmap.entries[idx].type = SEG_TYPE_EFI;
			break;
		}
		idx++;
	}
	efi_mmap.entry_count = idx;

	ST->BootServices->FreePool(buffer);

	return status;
}

int efi_get_mmap(struct memory_map *map)
{
	if (efi_mmap.entry_count) {
		*map = efi_mmap;
		return 0;
	}
	return 1;
}
