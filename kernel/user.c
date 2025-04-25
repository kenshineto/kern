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
	size_t npages, nvpages, nbytes;

	hdr = pcb->elf_segments[idx];
	nbytes = hdr.p_filesz;
	npages = (nbytes + PAGE_SIZE - 1) / PAGE_SIZE;
	nvpages = (hdr.p_memsz + PAGE_SIZE - 1) / PAGE_SIZE;

	if (npages < 1)
		return 0;

	if (hdr.p_type != PT_LOAD)
		return 0;

	// allocate memory in user process
	if (mem_alloc_pages_at(pcb->memctx, nvpages, (void *)hdr.p_vaddr,
						   F_WRITEABLE | F_UNPRIVILEGED) == NULL)
		return 1;

	// load data
	for (size_t i = 0; i < npages && nbytes; i++) {
		size_t bytes = PAGE_SIZE;
		if (nbytes < bytes)
			bytes = nbytes;
		mem_ctx_switch(kernel_mem_ctx); // disk_read is kernel internal
		memset(buf + bytes, 0, PAGE_SIZE - bytes);
		if (disk_read(disk, hdr.p_offset + i * PAGE_SIZE, bytes, buf))
			return 1;
		mem_ctx_switch(pcb->memctx);
		memcpy((char *)hdr.p_vaddr + i * PAGE_SIZE, buf, bytes);
		nbytes -= bytes;
	}

	mem_ctx_switch(kernel_mem_ctx);
	return 0;
}

static int user_load_segments(struct pcb *pcb, struct disk *disk)
{
	int ret = 0;

	for (int i = 0; i < pcb->n_elf_segments; i++)
		if ((ret = user_load_segment(pcb, disk, i)))
			return ret;
	return 0;
}

static int validate_elf_hdr(struct pcb *pcb)
{
	Elf64_Ehdr *ehdr = &pcb->elf_header;

	if (strncmp((const char *)ehdr->e_ident, ELFMAG, SELFMAG)) {
		WARN("Invalid ELF File.\n");
		return 1;
	}

	if (ehdr->e_ident[EI_CLASS] != ELFCLASS64) {
		WARN("Unsupported ELF Class.\n");
		return 1;
	}

	if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
		ERROR("Unsupported ELF File byte order.\n");
		return 1;
	}

	if (ehdr->e_machine != EM_X86_64) {
		WARN("Unsupported ELF File target.\n");
		return 1;
	}

	if (ehdr->e_ident[EI_VERSION] != EV_CURRENT) {
		WARN("Unsupported ELF File version.\n");
		return 1;
	}

	if (ehdr->e_phnum > N_ELF_SEGMENTS) {
		WARN("Too many ELF segments.\n");
		return 1;
	}

	if (ehdr->e_type != ET_EXEC) {
		ERROR("Unsupported ELF File type.\n");
		return 1;
	}

	return 0;
}

static int user_load_elf(struct pcb *pcb, struct disk *disk)
{
	int ret = 0;

	ret = disk_read(disk, 0, sizeof(Elf64_Ehdr), &pcb->elf_header);
	if (ret < 0)
		return 1;

	if (validate_elf_hdr(pcb))
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
