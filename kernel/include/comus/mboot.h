/**
 * @file mboot.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * Multiboot functions
 */

#ifndef MBOOT_H_
#define MBOOT_H_

#include <comus/memory.h>
#include <efi.h>

/**
 * Load the multiboot information passed from the bootloader
 */
void mboot_init(long magic, volatile void *mboot);

/**
 * Get the memory map from multiboot
 */
int mboot_get_mmap(struct memory_map *map);

/**
 * Get the ACPI rsdp addr from multiboot
 */
void *mboot_get_rsdp(void);

/**
 * Get elf symbols from multiboot
 */
const char *mboot_get_elf_sym(uint64_t addr);

/**
 * Returns EFI64 system table
 */
EFI_SYSTEM_TABLE *mboot_get_efi_st(void);

/**
 * Returns EFI64 image handle
 */
EFI_HANDLE mboot_get_efi_hdl(void);

/**
 * Returns the pointer to the loaded init ram disk with size given by len
 */
void *mboot_get_initrd(size_t *len);

#endif /* mboot.h */
