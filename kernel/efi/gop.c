#include <lib.h>
#include <efi.h>
#include <comus/efi.h>

static EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;

#define MAX_H_RES 1920

EFI_STATUS efi_load_gop(EFI_SYSTEM_TABLE *ST)
{
	EFI_STATUS status = EFI_SUCCESS;
	EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
	UINTN SizeOfInfo, numModes, nativeMode;

	status = ST->BootServices->LocateProtocol(&gopGuid, NULL, (void **)&gop);
	if (EFI_ERROR(status))
		return status;

	status = gop->QueryMode(gop, gop->Mode == NULL ? 0 : gop->Mode->Mode,
							&SizeOfInfo, &info);
	if (status == EFI_NOT_STARTED)
		status = gop->SetMode(gop, 0);
	if (EFI_ERROR(status))
		return status;

	nativeMode = gop->Mode->Mode;
	numModes = gop->Mode->MaxMode;

	// find the best mode
	UINTN best = nativeMode;
	UINTN width = 0;
	for (UINTN i = 0; i < numModes; i++) {
		status = gop->QueryMode(gop, i, &SizeOfInfo, &info);
		if (info->PixelFormat != PixelBlueGreenRedReserved8BitPerColor &&
			info->PixelFormat != PixelRedGreenBlueReserved8BitPerColor)
			continue;
		if (info->HorizontalResolution > MAX_H_RES)
			continue;
		if (info->HorizontalResolution > width) {
			width = info->HorizontalResolution;
			best = i;
		}
	}

	gop->SetMode(gop, best);
	if (EFI_ERROR(status))
		return status;

	return status;
}

EFI_GRAPHICS_OUTPUT_PROTOCOL *efi_get_gop(void)
{
	return gop;
}
