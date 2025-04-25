#include "lib/kio.h"
#include <comus/fs.h>
#include <comus/procs.h>
#include <comus/memory.h>
#include <comus/user.h>
#include <elf.h>

/// FIXME: the following code is using direct
/// disk access instead of file access, this is
/// because filesystems are not yet implemented.
/// This MUST be changed once we have files.
/// - Freya

#define USER_STACK_TOP 0x800000000000
#define USER_STACK_LEN (4 * PAGE_SIZE)

static int user_load_segment(struct pcb *pcb, struct disk *disk, int idx)
{
	uint8_t buf[PAGE_SIZE];
	Elf64_Phdr hdr;
	size_t npages;
	int ret = 0;

	hdr = pcb->elf_segments[idx];
	npages = (hdr.p_filesz + PAGE_SIZE - 1) / PAGE_SIZE;

	// allocate memory in user process
	if (mem_alloc_pages_at(pcb->memctx, npages, (void *)hdr.p_vaddr,
						   F_WRITEABLE | F_UNPRIVILEGED) == NULL)
		return 1;

	// load data
	for (size_t i = 0; i < npages; i++) {
		mem_ctx_switch(kernel_mem_ctx); // disk_read is kernel internal
		ret = disk_read(disk, hdr.p_offset + i * PAGE_SIZE, PAGE_SIZE, buf);
		if (ret < 0)
			break;
		mem_ctx_switch(pcb->memctx);
		memcpy((char *)hdr.p_vaddr + i * PAGE_SIZE, buf, PAGE_SIZE);
	}

	mem_ctx_switch(kernel_mem_ctx);
	return ret;
}

static int user_load_segments(struct pcb *pcb, struct disk *disk)
{
	int ret = 0;

	for (int i = 0; i < pcb->n_elf_segments; i++)
		if ((ret = user_load_segment(pcb, disk, i)))
			return ret;
	return 0;
}

static int user_load_elf(struct pcb *pcb, struct disk *disk)
{
	int ret = 0;

	ret = disk_read(disk, 0, sizeof(Elf64_Ehdr), &pcb->elf_header);
	if (ret < 0)
		return 1;

	if (pcb->elf_header.e_phnum > N_ELF_SEGMENTS)
		return 1;

	pcb->n_elf_segments = pcb->elf_header.e_phnum;
	ret = disk_read(disk, pcb->elf_header.e_phoff,
					sizeof(Elf64_Phdr) * pcb->elf_header.e_phnum,
					&pcb->elf_segments);
	if (ret < 0)
		return 1;

	return 0;
}

static int user_setup_stack(struct pcb *pcb)
{
	// allocate stack
	if (mem_alloc_pages_at(pcb->memctx, USER_STACK_LEN / PAGE_SIZE,
						   (void *)(USER_STACK_TOP - USER_STACK_LEN),
						   F_WRITEABLE | F_UNPRIVILEGED) == NULL)
		return 1;

	// setup initial context save area
	pcb->regs = (struct cpu_regs *)(USER_STACK_TOP - sizeof(struct cpu_regs));
	mem_ctx_switch(pcb->memctx);
	memset(pcb->regs, 0, sizeof(struct cpu_regs));
	pcb->regs->rip = pcb->elf_header.e_entry;
	pcb->regs->cs = 0x18 | 3;
	pcb->regs->rflags = (1 << 9);
	pcb->regs->rsp = USER_STACK_TOP;
	pcb->regs->ss = 0x20 | 3;
	mem_ctx_switch(kernel_mem_ctx);

	return 0;
}

int user_load(struct pcb *pcb, struct disk *disk)
{
	// check inputs
	if (pcb == NULL || disk == NULL)
		return 1;

	pcb->regs = NULL;

	// allocate memory context
	pcb->memctx = mem_ctx_alloc();
	if (pcb->memctx == NULL)
		goto fail;

	// load elf information
	if (user_load_elf(pcb, disk))
		goto fail;

	// load segments into memory
	if (user_load_segments(pcb, disk))
		goto fail;

	// setup process stack
	if (user_setup_stack(pcb))
		goto fail;

	// success
	return 0;

fail:
	user_cleanup(pcb);
	return 1;
}

void user_cleanup(struct pcb *pcb)
{
	if (pcb == NULL)
		return;

	mem_ctx_free(pcb->memctx);
	pcb->memctx = NULL;
}
