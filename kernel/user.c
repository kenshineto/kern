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

#define BLOCK_SIZE (PAGE_SIZE * 1000)
static uint8_t *load_buffer = NULL;

#define USER_CODE 0x18
#define USER_DATA 0x20
#define RING3 3

static int user_load_segment(struct pcb *pcb, struct file *file, int idx)
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
						   F_WRITEABLE | F_UNPRIVILEGED) == NULL) {
		ERROR("Could not allocate memory for elf segment");
		return 1;
	}

	mapADDR = kmapuseraddr(pcb->memctx, (void *)hdr.p_vaddr, mem_bytes);
	if (mapADDR == NULL) {
		ERROR("Could load memory for elf segment");
		return 1;
	}

	// seek to start of segment
	if (file->seek(file, hdr.p_offset, SEEK_SET) < 0) {
		ERROR("Could not load elf segment");
		return 1;
	}

	// load data
	size_t total_read = 0;
	while (total_read < file_bytes) {
		size_t read = BLOCK_SIZE;
		if (read > file_bytes - total_read)
			read = file_bytes - total_read;
		TRACE("Reading %zu bytes...", read);
		if ((read = file->read(file, load_buffer, read)) < 1) {
			kunmapaddr(mapADDR);
			ERROR("Could not load elf segment");
			return 1;
		}
		memcpyv(mapADDR + total_read, load_buffer, read);
		total_read += read;
	}

	// update heap end
	if (hdr.p_vaddr + mem_pages * PAGE_SIZE > (uint64_t)pcb->heap_start)
		pcb->heap_start = (void *)(hdr.p_vaddr + mem_pages * PAGE_SIZE);

	kunmapaddr(mapADDR);
	return 0;
}

static int user_load_segments(struct pcb *pcb, struct file *file)
{
	int ret = 0;

	pcb->heap_start = NULL;
	pcb->heap_len = 0;

	if (load_buffer == NULL) {
		load_buffer = kalloc(BLOCK_SIZE);
		if (load_buffer == NULL) {
			ERROR("Could not allocate user load buffer");
			return 1;
		}
	}

	TRACE("Loading %u elf segments", pcb->n_elf_segments);
	for (int i = 0; i < pcb->n_elf_segments; i++)
		if ((ret = user_load_segment(pcb, file, i)))
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
		ERROR("Invalid ELF File.");
		return 1;
	}

	if (ehdr->e_ident[EI_CLASS] != ELFCLASS64) {
		ERROR("Unsupported ELF Class.");
		return 1;
	}

	if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
		ERROR("Unsupported ELF File byte order.");
		return 1;
	}

	if (ehdr->e_machine != EM_X86_64) {
		ERROR("Unsupported ELF File target.");
		return 1;
	}

	if (ehdr->e_ident[EI_VERSION] != EV_CURRENT) {
		ERROR("Unsupported ELF File version.");
		return 1;
	}

	if (ehdr->e_phnum > N_ELF_SEGMENTS) {
		ERROR("Too many ELF segments.");
		return 1;
	}

	if (ehdr->e_type != ET_EXEC) {
		ERROR("Unsupported ELF File type.");
		return 1;
	}

	return 0;
}

static int user_load_elf(struct pcb *pcb, struct file *file)
{
	int ret = 0;

	if (file->seek(file, 0, SEEK_SET) < 0) {
		ERROR("Cannot read ELF header.");
		return 1;
	}
	ret = file->read(file, &pcb->elf_header, sizeof(Elf64_Ehdr));
	if (ret < 0) {
		ERROR("Cannot read ELF header.");
		return 1;
	}

	if (validate_elf_hdr(pcb))
		return 1;

	pcb->n_elf_segments = pcb->elf_header.e_phnum;
	if (file->seek(file, pcb->elf_header.e_phoff, SEEK_SET) < 0) {
		ERROR("Cannot read ELF segemts");
		return 1;
	}
	ret = file->read(file, &pcb->elf_segments,
					 sizeof(Elf64_Phdr) * pcb->elf_header.e_phnum);
	if (ret < 0) {
		ERROR("Cannot read ELF segemts");
		return 1;
	}

	return 0;
}

static int user_setup_stack(struct pcb *pcb, const char **args,
							mem_ctx_t args_ctx)
{
	/* args */
	int argbytes = 0;
	int argc = 0;

	mem_ctx_switch(args_ctx);

	while (args[argc] != NULL) {
		int n = strlen(args[argc]) + 1;
		if ((argbytes + n) > USER_STACK_LEN) {
			// oops - ignore this and any others
			break;
		}
		argbytes += n;
		argc++;
	}

	// round to nearest multiple of 8
	argbytes = (argbytes + 7) & 0xfffffffffffffff8;

	// allocate arg strings on kernel stack
	char argstrings[argbytes];
	char *argv[argc + 1];
	memset(argstrings, 0, sizeof(argstrings));
	memset(argv, 0, sizeof(argv));

	// Next, duplicate the argument strings, and create pointers to
	// each one in our argv.
	char *tmp = argstrings;
	for (int i = 0; i < argc; ++i) {
		int nb = strlen(args[i]) + 1;
		strcpy(tmp, args[i]);
		argv[i] = tmp;
		tmp += nb;
	}

	// trailing NULL pointer
	argv[argc] = NULL;

	mem_ctx_switch(kernel_mem_ctx);

	/* stack */
	if (mem_alloc_pages_at(pcb->memctx, USER_STACK_LEN / PAGE_SIZE,
						   (void *)(USER_STACK_TOP - USER_STACK_LEN),
						   F_WRITEABLE | F_UNPRIVILEGED) == NULL) {
		ERROR("Could not allocate user stack");
		return 1;
	}

	/* regs */
	memset(&pcb->regs, 0, sizeof(struct cpu_regs));

	// pgdir
	pcb->regs.cr3 = (uint64_t)mem_ctx_pgdir(pcb->memctx);
	// segments
	pcb->regs.gs = USER_DATA | RING3;
	pcb->regs.fs = USER_DATA | RING3;
	pcb->regs.es = USER_DATA | RING3;
	pcb->regs.ds = USER_DATA | RING3;
	// registers
	pcb->regs.rdi = argc; // argc
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

int user_load(struct pcb *pcb, struct file *file, const char **args,
			  mem_ctx_t args_ctx)
{
	// check inputs
	if (pcb == NULL || file == NULL)
		return 1;

	// allocate memory context
	pcb->memctx = mem_ctx_alloc();
	if (pcb->memctx == NULL)
		goto fail;

	// load elf information
	if (user_load_elf(pcb, file))
		goto fail;

	// load segments into memory
	if (user_load_segments(pcb, file))
		goto fail;

	// setup process stack
	if (user_setup_stack(pcb, args, args_ctx))
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
