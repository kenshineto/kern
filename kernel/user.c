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

#define BLOCK_SIZE (PAGE_SIZE * 1000)
static uint8_t *load_buffer = NULL;

#define USER_CODE 0x18
#define USER_DATA 0x20
#define RING3 3

static int user_load_segment(struct pcb *pcb, struct disk *disk, int idx)
{
	Elf64_Phdr hdr;
	size_t mem_bytes, mem_pages;
	size_t file_bytes, file_pages;
	uint8_t *mapADDR;

	hdr = pcb->elf_segments[idx];

	// return if this is not a lodable segment
	if (hdr.p_type != PT_LOAD)
		return 0;

	mem_bytes = hdr.p_memsz;
	file_bytes = hdr.p_filesz;

	// we cannot read more data to less memory
	if (file_bytes > mem_bytes)
		return 1;

	mem_pages = (mem_bytes + PAGE_SIZE - 1) / PAGE_SIZE;
	file_pages = (file_bytes + PAGE_SIZE - 1) / PAGE_SIZE;

	// return if were reading no data
	if (file_pages < 1)
		return 0;

	// allocate memory in user process
	if (mem_alloc_pages_at(pcb->memctx, mem_pages, (void *)hdr.p_vaddr,
						   F_WRITEABLE | F_UNPRIVILEGED) == NULL)
		return 1;

	mapADDR = kmapuseraddr(pcb->memctx, (void *)hdr.p_vaddr, mem_bytes);
	if (mapADDR == NULL)
		return 1;

	// load data
	size_t total_read = 0;
	while (total_read < file_bytes) {
		size_t read = BLOCK_SIZE;
		if (read > file_bytes - total_read)
			read = file_bytes - total_read;
		if ((read = disk_read(disk, hdr.p_offset + total_read, read,
							  load_buffer)) < 1) {
			kunmapaddr(mapADDR);
			return 1;
		}
		memcpy(mapADDR + total_read, load_buffer, read);
		total_read += read;
	}

	// update heap end
	if (hdr.p_vaddr + mem_pages * PAGE_SIZE > (uint64_t)pcb->heap_start)
		pcb->heap_start = (void *)(hdr.p_vaddr + mem_pages * PAGE_SIZE);

	kunmapaddr(mapADDR);
	return 0;
}

static int user_load_segments(struct pcb *pcb, struct disk *disk)
{
	int ret = 0;

	pcb->heap_start = NULL;
	pcb->heap_len = 0;

	if (load_buffer == NULL)
		if ((load_buffer = kalloc(BLOCK_SIZE)) == NULL)
			return 1;

	for (int i = 0; i < pcb->n_elf_segments; i++)
		if ((ret = user_load_segment(pcb, disk, i)))
			return ret;

	if (pcb->heap_start == NULL) {
		WARN("No loadable ELF segments found.");
		return 1;
	};

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

	memset(&pcb->regs, 0, sizeof(struct cpu_regs));

	// pgdir
	pcb->regs.cr3 = (uint64_t)mem_ctx_pgdir(pcb->memctx);
	// segments
	pcb->regs.gs = USER_DATA | RING3;
	pcb->regs.fs = USER_DATA | RING3;
	pcb->regs.es = USER_DATA | RING3;
	pcb->regs.ds = USER_DATA | RING3;
	// registers
	pcb->regs.rdi = 0; // argc
	pcb->regs.rsi = 0; // argv
	// intruction pointer
	pcb->regs.rip = pcb->elf_header.e_entry;
	// code segment
	pcb->regs.cs = USER_CODE | RING3;
	// rflags
	pcb->regs.rflags = (1 << 9);
	// stack pointer
	pcb->regs.rsp = USER_STACK_TOP;
	// stack segment
	pcb->regs.ss = USER_DATA | RING3;

	return 0;
}

int user_load(struct pcb *pcb, struct disk *disk)
{
	// check inputs
	if (pcb == NULL || disk == NULL)
		return 1;

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

struct pcb *user_clone(struct pcb *pcb)
{
	struct pcb *child;

	if (pcb_alloc(&child))
		return NULL;

	// copy context
	memcpy(&child->regs, &pcb->regs, sizeof(struct cpu_regs));
	child->memctx = mem_ctx_clone(pcb->memctx, true);
	if (child->memctx == NULL)
		return NULL;

	// set metadata
	child->parent = pcb;
	child->state = PROC_STATE_READY;
	child->priority = pcb->priority;
	child->ticks = 0;

	// copy heap
	child->heap_start = pcb->heap_start;
	child->heap_len = pcb->heap_len;

	// copy elf data
	memcpy(&child->elf_header, &pcb->elf_header, sizeof(Elf64_Ehdr));
	memcpy(&child->elf_segments, &pcb->elf_segments, sizeof(Elf64_Ehdr));
	child->n_elf_segments = pcb->n_elf_segments;

	return child;
}

void user_cleanup(struct pcb *pcb)
{
	if (pcb == NULL)
		return;

	mem_ctx_free(pcb->memctx);
	pcb->memctx = NULL;
}
