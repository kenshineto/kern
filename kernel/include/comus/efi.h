/**
 * @file efi.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * EFI functions
 */

#ifndef EFI_H_
#define EFI_H_

#include <comus/memory.h>
#include <efi.h>

/**
 * Initalize EFI structures
 */
void efi_init(EFI_HANDLE IH, EFI_SYSTEM_TABLE *ST);

/**
 * Get the memory map from multiboot
 */
int efi_get_mmap(struct memory_map *map);

/**
 * Get graphics output protocol
 */
EFI_GRAPHICS_OUTPUT_PROTOCOL *efi_get_gop(void);

/**
 * Report EFI system status
 */
void efi_report(void);

#endif /* efi.h */
