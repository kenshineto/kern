/**
** @file	user.c
**
** @author	CSCI-452 class of 20245
**
** @brief	User-level code manipulation routines
*/

#define KERNEL_SRC

#include <common.h>

#include <bootstrap.h>
#include <elf.h>
#include <user.h>
#include <vm.h>

/*
** PRIVATE DEFINITIONS
*/

/*
** PRIVATE DATA TYPES
*/

/*
** PRIVATE GLOBAL VARIABLES
*/

/*
** PUBLIC GLOBAL VARIABLES
*/

/*
** Location of the "user blob" in memory.
**
** These variables are filled in by the code in startup.S using values
** passed to it from the bootstrap.
**
** These are visible so that the startup code can find them.
*/
uint16_t user_offset; // byte offset from the segment base
uint16_t user_segment; // segment base address
uint16_t user_sectors; // number of 512-byte sectors it occupies

header_t *user_header; // filled in by the user_init routine
prog_t *prog_table; // filled in by the user_init routine

/*
** PRIVATE FUNCTIONS
*/

#if TRACING_ELF

/*
** This is debugging support code; if not debugging the ELF
** handling code, it won't be compiled into the kernel.
*/

// buffer used by some of these functions
static char ebuf[16];

/*
** File header functions
*/

// interpret the file class
static const char *fh_eclass(e32_si class)
{
	switch (class) {
	case ELF_CLASS_NONE:
		return ("None");
		break;
	case ELF_CLASS_32:
		return ("EC32");
		break;
	case ELF_CLASS_64:
		return ("EC64");
		break;
	}
	return ("????");
}

// interpret the data encoding
static const char *fh_edata(e32_si data)
{
	switch (data) {
	case ELF_DATA_NONE:
		return ("Invd");
		break;
	case ELF_DATA_2LSB:
		return ("2CLE");
		break;
	case ELF_DATA_2MSB:
		return ("2CBE");
		break;
	}
	return ("????");
}

// interpret the file type
static const char *fh_htype(e32_h type)
{
	switch (type) {
	case ET_NONE:
		return ("none");
		break;
	case ET_REL:
		return ("rel");
		break;
	case ET_EXEC:
		return ("exec");
		break;
	case ET_DYN:
		return ("dyn");
		break;
	case ET_CORE:
		return ("core");
		break;
	default:
		if (type >= ET_LO_OS && type <= ET_HI_OS)
			return ("OSsp");
		else if (type >= ET_LO_CP && type <= ET_HI_CP)
			return ("CPsp");
	}
	sprint(ebuf, "0x%04x", type);
	return ((const char *)ebuf);
}

// interpret the machine type
static const char *fh_mtype(e32_h machine)
{
	switch (machine) {
	case EM_NONE:
		return ("None");
		break;
	case EM_386:
		return ("386");
		break;
	case EM_ARM:
		return ("ARM");
		break;
	case EM_X86_64:
		return ("AMD64");
		break;
	case EM_AARCH64:
		return ("AARCH64");
		break;
	case EM_RISCV:
		return ("RISC-V");
		break;
	}
	return ("Other");
}

// dump the program header
static void dump_fhdr(elfhdr_t *hdr)
{
	cio_puts("File header: magic ");
	for (int i = EI_MAG0; i <= EI_MAG3; ++i)
		put_char_or_code(hdr->e_ident.bytes[i]);
	cio_printf(" class %s", fh_eclass(hdr->e_ident.f.class));
	cio_printf(" enc %s", fh_edata(hdr->e_ident.f.data));
	cio_printf(" ver %u\n", hdr->e_ident.f.version);
	cio_printf(" type %s", fh_htype(hdr->e_type));
	cio_printf(" mach %s", fh_mtype(hdr->e_machine));
	cio_printf(" vers %d", hdr->e_version);
	cio_printf(" entr %08x\n", hdr->e_entry);

	cio_printf(" phoff %08x", hdr->e_phoff);
	cio_printf(" shoff %08x", hdr->e_shoff);
	cio_printf(" flags %08x", (uint32_t)hdr->e_flags);
	cio_printf(" ehsize %u\n", hdr->e_ehsize);
	cio_printf(" phentsize %u", hdr->e_phentsize);
	cio_printf(" phnum %u", hdr->e_phnum);
	cio_printf(" shentsize %u", hdr->e_shentsize);
	cio_printf(" shnum %u", hdr->e_shnum);
	cio_printf(" shstrndx %u\n", hdr->e_shstrndx);
}

/*
** Program header functions
*/

// categorize the header type
static const char *ph_type(e32_w type)
{
	switch (type) {
	case PT_NULL:
		return ("Unused");
		break;
	case PT_LOAD:
		return ("Load");
		break;
	case PT_DYNAMIC:
		return ("DLI");
		break;
	case PT_INTERP:
		return ("Interp");
		break;
	case PT_NOTE:
		return ("Aux");
		break;
	case PT_SHLIB:
		return ("RSVD");
		break;
	case PT_PHDR:
		return ("PTentry");
		break;
	case PT_TLS:
		return ("TLS");
		break;
	default:
		if (type >= PT_LO_OS && type <= PT_HI_OS)
			return ("OSsp");
		else if (type >= PT_LO_CP && type <= PT_HI_CP)
			return ("CPsp");
	}
	sprint(ebuf, "0x%08x", type);
	return ((const char *)ebuf);
}

// report the individual flags
static void ph_flags(e32_w flags)
{
	if ((flags & PF_R) != 0)
		cio_putchar('R');
	if ((flags & PF_W) != 0)
		cio_putchar('W');
	if ((flags & PF_E) != 0)
		cio_putchar('X');
}

// dump a program header
static void dump_phdr(elfproghdr_t *hdr, int n)
{
	cio_printf("Prog header %d, type %s\n", n, ph_type(hdr->p_type));
	cio_printf(" offset %08x", hdr->p_offset);
	cio_printf(" va %08x", hdr->p_va);
	cio_printf(" pa %08x\n", hdr->p_pa);
	cio_printf(" filesz %08x", hdr->p_filesz);
	cio_printf(" memsz %08x", hdr->p_memsz);
	cio_puts(" flags ");
	ph_flags(hdr->p_flags);
	cio_printf(" align %08x", hdr->p_align);
	cio_putchar('\n');
}

/*
** Section header functions
*/

// interpret the header type
static const char *sh_type(e32_w type)
{
	switch (type) {
	case SHT_NULL:
		return ("Unused");
		break;
	case SHT_PROGBITS:
		return ("Progbits");
		break;
	case SHT_SYMTAB:
		return ("Symtab");
		break;
	case SHT_STRTAB:
		return ("Strtab");
		break;
	case SHT_RELA:
		return ("Rela");
		break;
	case SHT_HASH:
		return ("Hash");
		break;
	case SHT_DYNAMIC:
		return ("Dynamic");
		break;
	case SHT_NOTE:
		return ("Note");
		break;
	case SHT_NOBITS:
		return ("Nobits");
		break;
	case SHT_REL:
		return ("Rel");
		break;
	case SHT_SHLIB:
		return ("Shlib");
		break;
	case SHT_DYNSYM:
		return ("Dynsym");
		break;
	default:
		if (type >= SHT_LO_CP && type <= SHT_HI_CP)
			return ("CCsp");
		else if (type >= SHT_LO_US && type <= SHT_HI_US)
			return ("User");
	}
	sprint(ebuf, "0x%08x", type);
	return ((const char *)ebuf);
}

// report the various flags
static void sh_flags(unsigned int flags)
{
	if ((flags & SHF_WRITE) != 0)
		cio_putchar('W');
	if ((flags & SHF_ALLOC) != 0)
		cio_putchar('A');
	if ((flags & SHF_EXECINSTR) != 0)
		cio_putchar('X');
	if ((flags & SHF_MERGE) != 0)
		cio_putchar('M');
	if ((flags & SHF_STRINGS) != 0)
		cio_putchar('S');
	if ((flags & SHF_INFO_LINK) != 0)
		cio_putchar('L');
	if ((flags & SHF_LINK_ORDER) != 0)
		cio_putchar('o');
	if ((flags & SHF_OS_NONCON) != 0)
		cio_putchar('n');
	if ((flags & SHF_GROUP) != 0)
		cio_putchar('g');
	if ((flags & SHF_TLS) != 0)
		cio_putchar('t');
}

// dump a section header
ATTR_UNUSED
static void dump_shdr(elfsecthdr_t *hdr, int n)
{
	cio_printf("Sect header %d, type %d (%s), name %s\n", n, hdr->sh_type,
			   sh_type(hdr->sh_type));
	cio_printf(" flags %08x ", (uint32_t)hdr->sh_flags);
	sh_flags(hdr->sh_flags);
	cio_printf(" addr %08x", hdr->sh_addr);
	cio_printf(" offset %08x", hdr->sh_offset);
	cio_printf(" size %08x\n", hdr->sh_size);
	cio_printf(" link %08x", hdr->sh_link);
	cio_printf(" info %08x", hdr->sh_info);
	cio_printf(" align %08x", hdr->sh_addralign);
	cio_printf(" entsz %08x\n", hdr->sh_entsize);
}
#endif

/**
** read_phdrs(addr,phoff,phentsize,phnum)
**
** Parses the ELF program headers and each segment described into memory.
**
** @param hdr  Pointer to the program header
** @param pcb  Pointer to the PCB (and its PDE)
**
** @return status of the attempt:
**     SUCCESS         everything loaded correctly
**     E_LOAD_LIMIT    more than N_LOADABLE PT_LOAD sections
**     other           status returned from vm_add()
*/
static int read_phdrs(elfhdr_t *hdr, pcb_t *pcb)
{
	// sanity check
	assert1(hdr != NULL);
	assert2(pcb != NULL);

#if TRACING_USER
	cio_printf("read_phdrs(%08x,%08x)\n", (uint32_t)hdr, (uint32_t)pcb);
#endif

	// iterate through the program headers
	uint_t nhdrs = hdr->e_phnum;

	// pointer to the first header table entry
	elfproghdr_t *curr = (elfproghdr_t *)((uint32_t)hdr + hdr->e_phoff);

	// process them all
	int loaded = 0;
	for (uint_t i = 0; i < nhdrs; ++i, ++curr) {
#if TRACING_ELF
		dump_phdr(curr, i);
#endif
		if (curr->p_type != PT_LOAD) {
			// not loadable --> we'll skip it
			continue;
		}

		if (loaded >= N_LOADABLE) {
#if TRACING_USER
			cio_puts(" LIMIT\n");
#endif
			return E_LOAD_LIMIT;
		}

		// set a pointer to the bytes within the object file
		char *data = (char *)(((uint32_t)hdr) + curr->p_offset);
#if TRACING_USER
		cio_printf(" data @ %08x", (uint32_t)data);
#endif

		// copy the pages into memory
		int stat = vm_add(pcb->pdir, curr->p_flags & PF_W, false,
						  (char *)curr->p_va, curr->p_memsz, data,
						  curr->p_filesz);
		if (stat != SUCCESS) {
			// TODO what else should we do here? check for memory leak?
			return stat;
		}

		// set the section table entry in the PCB
		pcb->sects[loaded].length = curr->p_memsz;
		pcb->sects[loaded].addr = curr->p_va;
#if TRACING_USER
		cio_printf(" loaded %u @ %08x\n", pcb->sects[loaded].length,
				   pcb->sects[loaded].addr);
#endif
		++loaded;
	}

	return SUCCESS;
}

/**
** Name:	stack_setup
**
** Set up the stack for a new process
**
** @param pcb    Pointer to the PCB for the process
** @param entry  Entry point for the new process
** @param args   Argument vector to be put in place
** @param sys    Is the argument vector from kernel code?
**
** @return A (user VA) pointer to the context_t on the stack, or NULL
*/
static context_t *stack_setup(pcb_t *pcb, uint32_t entry, const char **args,
							  bool_t sys)
{
#if TRACING_USER
	cio_printf("stksetup: pcb %08x, entry %08x, args %08x\n", (uint32_t)pcb,
			   entry, (uint32_t)args);
#endif

	/*
	** First, we need to calculate the space we'll need for the argument
	** vector and strings.
	**
	** Keeping track of kernel vs. user VAs is tricky, so we'll use
	** a prefix on variable names: kv_* is a kernel virtual address;
	** uv_* is a user virtual address.
	**
	** We rely on the C standard, section 6.7.8, to clear these arrays:
	**
	**   "21 If there are fewer initializers in a brace-enclosed list
	**       than there are elements or members of an aggregate, or
	**       fewer characters in a string literal used to initialize an
	**       array of known size than there are elements in the array,
	**       the remainder of the aggregate shall be initialized
	**       implicitly the same as objects that have static storage
	**       duration."
	*/

	int argbytes = 0; // total length of arg strings
	int argc = 0; // number of argv entries
	const char *kv_strs[N_ARGS] = { 0 }; // converted user arg string pointers
	int strlengths[N_ARGS] = { 0 }; // length of each string
	const char *uv_argv[N_ARGS] = { 0 }; // argv pointers

	/*
	** IF the argument list given to us came from  user code, we need
	** to convert its address and the addresses it contains to kernel
	** VAs; otherwise, we can use them directly.
	*/
	char **kv_args = sys ? args : vm_uva2kva(pcb->pdir, (void *)args);

	while (kv_args[argc] != NULL) {
		kv_strs[argc] = sys ? args[argc] :
							  vm_uva2kva(pcb->pdir, (void *)(kv_args[argc]));
		strlengths[argc] = strlen(kv_strs[argc]) + 1;
		// can't go over one page in size
		if ((argbytes + strlengths[argc]) > SZ_PAGE) {
			// oops - ignore this and any others
			break;
		}
		argbytes += strlengths[argc];
		++argc;
	}

	// Round up the byte count to the next multiple of four.
	argbytes = (argbytes + 3) & MOD4_MASK;

	/*
	** The pages for the stack were cleared when they were allocated,
	** so we don't need to remember to do that.
	**
	** We reserve one longword at the bottom of the stack to hold a
	** pointer to where argv is on the stack.
	**
	** The user code was linked with a startup function that defines
	** the entry point (_start), calls main(), and then calls exit()
	** if main() returns. We need to set up the stack this way:
	**
	**      esp ->  context      <- context save area
	**              ...          <- context save area
	**              context      <- context save area
	**              entry_pt     <- return address for the ISR
	**              argc         <- argument count for main()
	**         /->  argv         <- argv pointer for main()
	**         |     ...         <- argv array w/trailing NULL
	**         |     ...         <- argv character strings
	**         \--- ptr          <- last word in stack
	**
	** Stack alignment rules for the SysV ABI i386 supplement dictate that
	** the 'argc' parameter must be at an address that is a multiple of 16;
	** see below for more information.
	*/

	/*
	** Find the user stack. The PDE entry for user address space points
	** to a page table for the first 4MB of the address space, but the
	** "pointer" there a physical frame address.
	*/
	pde_t *kv_userpt = (pde_t *)P2V(PTE_ADDR(pcb->pdir[USER_PDE]));
	assert(kv_userpt != NULL);

	/*
	** The final entries in that PMT are for the pages of the user stack.
	** Grab the address of the frame for the last one.  (Again, we need
	** to convert it to a virtual address we can use.)
	*/

	// the PMT entry for that page
	pte_t pmt_entry = kv_userpt[USER_STK_LAST_PTE];
	assert(IS_PRESENT(pmt_entry));

	// kernel VA for the first byte following that page
	uint8_t *kv_ptr = (uint8_t *)P2V(PTE_ADDR(pmt_entry) + SZ_PAGE);
	assert(kv_ptr != NULL);

	// user VA for the first byte following that page
	uint32_t *uv_ptr = (uint32_t *)(USER_STACK_P2 + SZ_PAGE);

	// Pointers to where the arg strings should be filled in.
	uint32_t kv_strings = ((uint32_t)kv_ptr) - argbytes;
	uint32_t uv_strings = ((uint32_t)uv_ptr) - argbytes;

	// back the pointers up to the nearest word boundary; because we're
	// moving toward location 0, the nearest word boundary is just the
	// next smaller address whose low-order two bits are zeroes
	kv_strings &= MOD4_MASK;
	uv_strings &= MOD4_MASK;

	/*
	** Next, we need to copy over the data. Start by determining where
	** where 'argc' should go.
	**
	** Stack alignment is controlled by the SysV ABI i386 supplement,
	** version 1.2 (June 23, 2016), which states in section 2.2.2:
	**
	**   "The end of the input argument area shall be aligned on a 16
	**   (32 or 64, if __m256 or __m512 is passed on stack) byte boundary.
	**   In other words, the value (%esp + 4) is always a multiple of 16
	**   (32 or 64) when control is transferred to the function entry
	**   point. The stack pointer, %esp, always points to the end of the
	**   latest allocated stack frame."
	**
	** Isn't technical documentation fun?  Ultimately, this means that
	** the first parameter to main() should be on the stack at an address
	** that is a multiple of 16. In our case, that is 'argc'.
	**
	** The space needed for argc, argv, and the argv array itself is
	** argc + 3 words (argc+1 for the argv entries, plus one word each
	** for argc and argv).  We back up that much from 'strings'.
	*/

	int nwords = argc + 3;
	uint32_t *kv_acptr = ((uint32_t *)kv_strings) - nwords;
	uint32_t *uv_acptr = ((uint32_t *)uv_strings) - nwords;

	// back these up to multiple-of-16 addresses for stack alignment
	kv_acptr = (uint32_t *)(((uint32_t)kv_acptr) & MOD16_MASK);
	uv_acptr = (uint32_t *)(((uint32_t)uv_acptr) & MOD16_MASK);

	// the argv location
	uint32_t *kv_avptr = kv_acptr + 1;

	// the user address for the first argv entry
	uint32_t *uv_avptr = uv_acptr + 2;

	// Copy over the argv strings.
	for (int i = 0; i < argc; ++i) {
		// copy the string using kernel addresses
		strcpy((char *)kv_strings, kv_args[i]);

		// remember the user address where this string went
		uv_argv[i] = (char *)uv_strings;

		// adjust both string addresses
		kv_strings += strlengths[i];
		uv_strings += strlengths[i];
	}

	/*
	** Next, we copy in argc, argv, and the pointers. The stack will
	** look something like this:
	**
	**         kv_avptr
	** kv_acptr  |
	**     |     |
	**     v     v
	**    argc  argv  av[0] av[1] etc  NULL       str0   str1    etc.
	**   [....][....][....][....] ... [0000] ... [......0......0.........]
	**           |    ^ |    |                    ^      ^
	**           |    | |    |                    |      |
	**           ------ |    ---------------------|-------
	**                  ---------------------------
	*/

	// copy in 'argc'
	*kv_acptr = argc;

	// copy in 'argv'
	*kv_avptr++ = (uint32_t)uv_avptr;

	// now, the argv entries themselves
	for (int i = 0; i < argc; ++i) {
		*kv_avptr++ = (uint32_t)uv_argv[i];
	}

	// and the trailing NULL
	*kv_avptr = NULL;

	/*
	** Almost done!
	**
	** Now we need to set up the initial context for the executing
	** process.
	**
	** When this process is dispatched, the context restore code will
	** pop all the saved context information off the stack, including
	** the saved EIP, CS, and EFLAGS. We set those fields up so that
	** the interrupt "returns" to the entry point of the process.
	*/

	// Locate the context save area on the stack by backup up one
	// "context" from where the argc value is saved
	context_t *kv_ctx = ((context_t *)kv_acptr) - 1;
	uint32_t uv_ctx = (uint32_t)(((context_t *)uv_acptr) - 1);

	/*
	** We cleared the entire stack earlier, so all the context
	** fields currently contain zeroes.  We now need to fill in
	** all the important fields.
	**
	** Note: we don't need to set the ESP value for the process,
	** as the 'popa' that restores the general registers doesn't
	** actually restore ESP from the context area - it leaves it
	** where it winds up.
	*/

	kv_ctx->eflags = DEFAULT_EFLAGS; // IF enabled, IOPL 0
	kv_ctx->eip = entry; // initial EIP
	kv_ctx->cs = GDT_CODE; // segment registers
	kv_ctx->ss = GDT_STACK;
	kv_ctx->ds = kv_ctx->es = kv_ctx->fs = kv_ctx->gs = GDT_DATA;

	/*
	** Return the new context pointer to the caller.  It will be our
	** caller's responsibility to schedule this process.
	*/

	return ((context_t *)uv_ctx);
}

/*
** PUBLIC FUNCTIONS
*/

/**
** Name:	user_init
**
** Initializes the user support module.
*/
void user_init(void)
{
#if TRACING_INIT
	cio_puts(" User");
#endif

	// This is gross, but we need to get this information somehow.
	// Access the "user blob" data in the second bootstrap sector
	uint16_t *blobdata = (uint16_t *)P2V(USER_BLOB_DATA);
	user_offset = *blobdata++;
	user_segment = *blobdata++;
	user_sectors = *blobdata++;

#if TRACING_USER
	cio_printf("\nUser blob: %u sectors @ %04x:%04x", user_sectors,
			   user_segment, user_offset);
#endif

	// calculate the location of the user blob
	if (user_sectors > 0) {
		// calculate the address of the header
		user_header = (header_t *)(KERN_BASE + ((((uint_t)user_segment) << 4) +
												((uint_t)user_offset)));

		// the program table immediate follows the blob header
		prog_table = (prog_t *)(user_header + 1);

#if TRACING_USER
		cio_printf(", hdr %08x, %u progs, tbl %08x\n", (uint32_t)user_header,
				   user_header->num, (uint32_t)prog_table);
#endif

	} else {
		// too bad, so sad!
		user_header = NULL;
		prog_table = NULL;
#if TRACING_USER
		cio_putchar('\n');
#endif
	}
}

/**
** Name:	user_locate
**
** Locates a user program in the user code archive.
**
** @param what   The ID of the user program to find
**
** @return pointer to the program table entry in the code archive, or NULL
*/
prog_t *user_locate(uint_t what)
{
#if TRACING_USER
	cio_printf("ulocate: %u\n", what);
#endif

	// no programs if there is no blob!
	if (user_header == NULL) {
		return NULL;
	}

	// make sure this is a reasonable program to request
	if (what >= user_header->num) {
		// no such program!
		return NULL;
	}

	// find the entry in the program table
	prog_t *prog = &prog_table[what];

	// if there are no bytes, it's useless
	if (prog->size < 1) {
		return NULL;
	}

	// return the program table pointer
	return prog;
}

/**
** Name:    user_duplicate
**
** Duplicates the memory setup for an existing process.
**
** @param new   The PCB for the new copy of the program
** @param old   The PCB for the existing the program
**
** @return the status of the duplicate attempt
*/
int user_duplicate(pcb_t *new, pcb_t *old)
{
#if TRACING_USER
	cio_printf("udup: old %08x new %08x\n", (uint32_t)old, (uint32_t)new);
#endif

	// We need to do a recursive duplication of the process address
	// space of the current process. First, we create a new user
	// page directory. Next, we'll duplicate the USER_PDE page
	// table. Finally, we'll go through that table and duplicate
	// all the frames.

	// create the initial VM hierarchy
	pde_t *pdir = vm_mkuvm();
	if (pdir == NULL) {
		return E_NO_MEMORY;
	}
	new->pdir = pdir;

	// Next, add a USER_PDE page table that's a duplicate of the
	// current process' page table
	if (!vm_uvmdup(new->pdir, old->pdir)) {
		// check for memory leak?
		return E_NO_MEMORY;
	}

	// We don't do copy-on-write, so we must duplicate all the
	// individual page frames. Iterate through all the user-level
	// PDE entries, and replace the existing frames with duplicates.
	//
	// NOTE: we only deal with pdir[0] here, as we are limiting
	// the user address space to the first 4MB.  If the size of
	// the address space goes up, this code will need to be
	// modified to loop over the larger space.

	// pointer to the PMT for the user
	pte_t *pt = (pte_t *)(pdir[USER_PDE]);
	assert(pt != NULL);

	for (int i = 0; i < N_PTE; ++i) {
		// get the current entry from the PMT
		pte_t entry = *pt;

		// if this entry is present,
		if (IS_PRESENT(entry)) {
			// duplicate the frame pointed to by this PTE
			void *tmp = vm_pagedup((void *)PTE_ADDR(entry));

			// replace the old frame number with the new one
			*pt = (pte_t)(((uint32_t)tmp) | PERMS(entry));

		} else {
			*pt = 0;
		}
		++pt;
	}

	return SUCCESS;
}

/**
** Name:	user_load
**
** Loads a user program from the user code archive into memory.
** Allocates all needed frames and sets up the VM tables.
**
** @param ptab   A pointer to the program table entry to be loaded
** @param pcb    The PCB for the program being loaded
** @param args   The argument vector for the program
** @param sys    Is the argument vector from kernel code?
**
** @return the status of the load attempt
*/
int user_load(prog_t *ptab, pcb_t *pcb, const char **args, bool_t sys)
{
	// NULL pointers are bad!
	assert1(ptab != NULL);
	assert1(pcb != NULL);
	assert1(args != NULL);

#if TRACING_USER
	cio_printf("Uload: prog '%s' pcb %08x args %08x\n",
			   ptab->name[0] ? ptab->name : "?", (uint32_t)pcb, (uint32_t)args);
#endif

	// locate the ELF binary
	elfhdr_t *hdr = (elfhdr_t *)((uint32_t)user_header + ptab->offset);

#if TRACING_ELF
	cio_printf("Load: ptab %08x: '%s', off %08x, size %08x, flags %08x\n",
			   (uint32_t)ptab, ptab->name, ptab->offset, ptab->size,
			   ptab->flags);
	cio_printf("      args %08x:", (uint32_t)args);
	if (sys) {
		for (int i = 0; args[i] != NULL; ++i) {
			cio_printf(" [%d] %s", i, args[i]);
		}
	} else {
		char **kv_args = vm_uva2kva(pcb->pdir, args);
		for (int i = 0; kv_args[i] != NULL; ++i) {
			cio_printf(" [%d] %s", i,
					   (char *)vm_uva2kva(pcb->pdir, kv_args[i]));
		}
	}
	cio_printf("\n      pcb %08x (pid %u)\n", (uint32_t)pcb, pcb->pid);
	dump_fhdr(hdr);
#endif

	// verify the ELF header
	if (hdr->e_ident.f.magic != ELF_MAGIC) {
		return E_BAD_PARAM;
	}

	// allocate a page directory
	pcb->pdir = vm_mkuvm();
	if (pcb->pdir == NULL) {
		return E_NO_MEMORY;
	}

	// read all the program headers
	int stat = read_phdrs(hdr, pcb);
	if (stat != SUCCESS) {
		cio_printf("Uload: read_phdrs('%s') returned %d\n", ptab->name, stat);
		PANIC(0, "User_load: phdr read failed");
	}

	// next, set up the runtime stack - just like setting up loadable
	// sections, except nothing to copy
	stat =
		vm_add(pcb->pdir, true, false, (void *)USER_STACK, SZ_USTACK, NULL, 0);
	if (stat != SUCCESS) {
		cio_printf("Uload: vm_add('%s') stack returned %d\n", ptab->name, stat);
		PANIC(0, "user_load: vm_add stack failed");
	}

	// set up the command-line arguments
	pcb->context = stack_setup(pcb, hdr->e_entry, args, sys);

	return SUCCESS;
}

/**
** Name:	user_cleanup
**
** "Unloads" a user program. Deallocates all memory frames and
** cleans up the VM structures.
**
** @param pcb   The PCB of the program to be unloaded
*/
void user_cleanup(pcb_t *pcb)
{
#if TRACING_USER
	cio_printf("Uclean: %08x\n", (uint32_t)pcb);
#endif

	if (pcb == NULL) {
		// should this be an error?
		return;
	}

	vm_free(pcb->pdir);
	pcb->pdir = NULL;
}
