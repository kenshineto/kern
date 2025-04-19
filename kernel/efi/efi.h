#ifndef __EFI_H
#define __EFI_H

#include <efi.h>

EFI_STATUS efi_load_mmap(EFI_SYSTEM_TABLE *ST);
EFI_STATUS efi_load_gop(EFI_SYSTEM_TABLE *ST);

#endif /* efi.h */
