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

#define MBOOT_HEADER_MAGIC  0x36D76289

#define MBOOT_CMDLINE       1
#define MBOOT_MEMORY_MAP    6
#define MBOOT_FRAMEBUFFER   8
#define MBOOT_ELF_SYMBOLS   9
#define MBOOT_OLD_RSDP     14
#define MBOOT_NEW_RSDP     15

struct mboot_info {
	uint32_t total_size;
	uint32_t reserved;
	char tags[];
};

struct mboot_tag {
	uint32_t type;
	uint32_t size;
	char data[];
};

struct mboot_tag_elf_sections {
	uint32_t type;
	uint32_t size;
	uint16_t num;
	uint16_t entsize;
	uint16_t shndx;
	uint16_t reserved;
	char sections[];
};

struct mboot_tag_elf_sections_entry {
	uint32_t sh_name;
	uint32_t sh_type;
	uint64_t sh_flags;
	uint64_t sh_addr;
	uint64_t sh_offset;
	uint64_t sh_size;
	uint32_t sh_link;
	uint32_t sh_info;
	uint64_t sh_addralign;
	uint64_t sh_entsize;
};

struct mboot_mmap_entry {
	uint64_t addr;
	uint64_t len;
	uint32_t type;
	uint32_t zero;
};

struct mboot_tag_mmap {
	uint32_t type;
	uint32_t size;
	uint32_t entry_size;
	uint32_t entry_version;
	struct mboot_mmap_entry entries[];
};


struct mboot_tag_old_rsdp {
	uint32_t type;
	uint32_t size;
	char rsdp[];
};

struct mboot_tag_new_rsdp {
	uint32_t type;
	uint32_t size;
	char rsdp[];
};

struct mboot_tag_cmdline {
	uint32_t type;
	uint32_t size;
	uint8_t cmdline[];
};

struct mboot_tag_framebuffer {
	uint32_t type;
	uint32_t size;
	uint64_t framebuffer_addr;
	uint32_t framebuffer_pitch;
	uint32_t framebuffer_width;
	uint32_t framebuffer_height;
	uint8_t framebuffer_bpp;
	uint8_t framebuffer_type;
	uint16_t reserved;
};

void *locate_mboot_table(volatile void *mboot, uint32_t type);

#endif /* mboot.h */
