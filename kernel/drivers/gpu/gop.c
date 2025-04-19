#include <lib.h>
#include <comus/asm.h>
#include <comus/drivers/gpu.h>
#include <comus/drivers/gpu/gop.h>
#include <comus/efi.h>
#include <efi.h>

struct gpu gop_dev = { 0 };

static EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;

int gop_init(struct gpu **gpu_dev)
{
	gop = efi_get_gop();
	if (gop == NULL || gop->Mode == NULL)
		return 1;

	gop_dev.name = "GOP";
	gop_dev.width = gop->Mode->Info->HorizontalResolution;
	gop_dev.height = gop->Mode->Info->VerticalResolution;
	gop_dev.bit_depth = 32; // we only allow 8bit color in efi/gop.c
	gop_dev.framebuffer = kmapaddr((void *)gop->Mode->FrameBufferBase, NULL,
								   gop->Mode->FrameBufferSize, F_WRITEABLE);
	*gpu_dev = &gop_dev;

	return 1;
}
