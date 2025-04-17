#include "lib/kio.h"
#include <lib.h>
#include <elf.h>
#include <comus/mboot.h>

#include "mboot.h"

#define MULTIBOOT_TAG_TYPE_ELF_SECTIONS 9

struct multiboot_tag_elf_sections {
	uint32_t type;
	uint32_t size;
	uint32_t num;
	uint32_t entsize;
	uint32_t shndx;
	Elf64_Shdr sections[];
};

static struct multiboot_tag_elf_sections *elf = NULL;
static Elf64_Shdr *symtab = NULL;
static Elf64_Shdr *symstrtab = NULL;

static Elf64_Shdr *mboot_get_elf_sec(uint32_t sh_type)
{
	for (uint32_t i = 0; i < elf->num; i++) {
		Elf64_Shdr *ent = &elf->sections[i];
		if (ent->sh_type == sh_type)
			return ent;
	}

	return NULL;
}

static int mboot_load_elf(void)
{
	void *tag = locate_mboot_table(MULTIBOOT_TAG_TYPE_ELF_SECTIONS);
	if (tag == NULL)
		return 1;

	// found elf sections
	elf = (struct multiboot_tag_elf_sections *)tag;

	// load symtab
	if ((symtab = mboot_get_elf_sec(SHT_SYMTAB)) == NULL)
		return 1;

	// load strsymtab
	if ((symstrtab = mboot_get_elf_sec(symtab->sh_link)) == NULL)
		return 1;

	return 0;
}

const char *mboot_get_elf_sym(uint64_t addr)
{
	if (symstrtab == NULL)
		if (mboot_load_elf())
			return NULL;

	// walk symbol table
	Elf64_Sym *syms = (Elf64_Sym *)symtab->sh_addr;
	Elf64_Sym *best = NULL;
	for (uint32_t i = 0; i < symtab->sh_size / symtab->sh_entsize; i++) {
		Elf64_Sym *sym = &syms[i];
		if (sym->st_value < addr)
			continue;
		if (best == NULL || (best->st_value < sym->st_value))
			best = sym;
	}

	if (best != NULL) {
		char *buf = (char *)symstrtab->sh_addr;
		return &buf[best->st_name];
	}

	return "???";
}
