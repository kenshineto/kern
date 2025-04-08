/**
 * @file mboot.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * Internal multiboot structures
 */

#ifndef __MBOOT_H_
#define __MBOOT_H_

#include <lib.h>

#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36D76289

struct multiboot {
	uint32_t total_size;
	uint32_t reserved;
	char tags[];
};

struct multiboot_tag {
	uint32_t type;
	uint32_t size;
};

void *locate_mboot_table(uint32_t type);

#endif /* mboot.h */
