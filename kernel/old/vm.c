/**
** @file	vm.c
**
** @author	CSCI-452 class of 20245
**
** @brief	Kernel VM support
*/

#define KERNEL_SRC

#include <common.h>

#include <vm.h>
#include <vmtables.h>

#include <kmem.h>
#include <procs.h>
#include <x86/arch.h>
#include <x86/ops.h>

/*
** PUBLIC GLOBAL VARIABLES
*/

// created page directory for the kernel
pde_t *kpdir;

/*
** PRIVATE FUNCTIONS
*/

/**
** Name:	vm_isr
**
** Description: Page fault handler
**
** @param vector   Interrupt vector number
** @param code     Error code pushed onto the stack
*/
static void vm_isr(int vector, int code)
{
	// get whatever information we can from the fault
	pfec_t fault;
	fault.u = (uint32_t)code;
	uint32_t addr = r_cr2();

	// report what we found
	sprint(b256,
		   "** page fault @ 0x%08x %cP %c %cM %cRSV %c %cPK %cSS %cHLAT %cSGZ",
		   addr, fault.s.p ? ' ' : '!', fault.s.w ? 'W' : 'R',
		   fault.s.us ? 'U' : 'S', fault.s.rsvd ? ' ' : '!',
		   fault.s.id ? 'I' : 'D', fault.s.pk ? ' ' : '!',
		   fault.s.ss ? ' ' : '!', fault.s.hlat ? ' ' : '!',
		   fault.s.sgz ? ' ' : '!');

	// and give up
	PANIC(0, b256);
}

/**
** Name:	ptcount
**
** Count the number of each type of entry in a page table.
** Returns a 32-bit result containing two 16-bit counts:
**
**            Upper half          Lower half
**  PDIR:  # of 4MB entries    # of 'present' entries
**  PMT:               zero    # of 'present' entries
**
** The number of "not present" can be calculated from these.
**
** @param pt   Pointer to the page table
** @param dir  Is it a page directory (vs. a page table)?
*/
ATTR_UNUSED
static uint32_t ptcount(pte_t *ptr, bool_t dir)
{
	uint16_t n_np = 0, n_p = 0, n_lg = 0;

	for (int i = 0; i < N_PTE; ++i) {
		pde_t entry = *ptr++;
		if (!IS_PRESENT(entry)) {
			++n_np;
			continue;
		}
		if (dir && IS_LARGE(entry)) {
			++n_lg;
		} else {
			++n_p;
		}
	}

	// n_lg will be 0 for PMTs
	return (n_lg << 16) | n_p;
}

// decode a PDE
static void pde_prt(uint32_t level, uint32_t i, uint32_t entry, bool_t all)
{
	if (!IS_PRESENT(entry) && !all) {
		return;
	}

	// indent
	for (int n = 0; n <= level; ++n)
		cio_puts("  ");

	// line header
	cio_printf("[%03x] %08x", i, entry);

	// perms
	if (IS_LARGE(entry)) { // PS is 1
		if ((entry & PDE_PAT) != 0)
			cio_puts(" PAT");
		if ((entry & PDE_G) != 0)
			cio_puts(" G");
		cio_puts(" PS");
		if ((entry & PDE_D) != 0)
			cio_puts(" D");
	}
	if ((entry & PDE_A) != 0)
		cio_puts(" A");
	if ((entry & PDE_PCD) != 0)
		cio_puts(" CD");
	if ((entry & PDE_PWT) != 0)
		cio_puts(" WT");
	if ((entry & PDE_US) != 0)
		cio_puts(" U");
	if ((entry & PDE_RW) != 0)
		cio_puts(" W");

	// frame address
	cio_printf(" P --> %s %08x\n", IS_LARGE(entry) ? "Pg" : "PT",
			   PDE_ADDR(entry));
}

// decode a PTE
static void pte_prt(uint32_t level, uint32_t i, uint32_t entry, bool_t all)
{
	if (!IS_PRESENT(entry) && !all) {
		return;
	}

	// indent
	for (int n = 0; n <= level; ++n)
		cio_puts("  ");

	// line header
	cio_printf("[%03x] %08x", i, entry);

	// perms
	if ((entry & PTE_G) != 0)
		cio_puts(" G");
	if ((entry & PTE_PAT) != 0)
		cio_puts(" PAT");
	if ((entry & PTE_D) != 0)
		cio_puts(" D");
	if ((entry & PTE_A) != 0)
		cio_puts(" A");
	if ((entry & PTE_PCD) != 0)
		cio_puts(" CD");
	if ((entry & PTE_PWT) != 0)
		cio_puts(" WT");
	if ((entry & PTE_US) != 0)
		cio_puts(" U");
	if ((entry & PTE_RW) != 0)
		cio_puts(" W");

	cio_printf(" P --> Pg %08x\n", PTE_ADDR(entry));
}

/**
** Name:	ptdump
**
** Recursive helper for table hierarchy dump.
**
** @param level  Current hierarchy level
** @param pt     Page table to display
** @param dir    Is it a page directory (vs. a page table)?
** @param mode   How to display the entries
*/
ATTR_UNUSED
static void ptdump(uint_t level, void *pt, bool_t dir, enum vmmode_e mode)
{
	pte_t *ptr = (pte_t *)pt;

	// indent two spaces per level
	for (int n = 0; n < level; ++n)
		cio_puts("  ");

	cio_printf("%s at 0x%08x:", dir ? "PDir" : "PTbl", (uint32_t)pt);
	uint32_t nums = ptcount(ptr, dir);
	if (dir) {
		cio_printf(" 4MB=%u", (nums >> 16));
	}
	cio_printf(" P=%u !P=%u\n", nums & 0xffff,
			   N_PTE - ((nums >> 16) + (nums & 0xffff)));

	for (uint32_t i = 0; i < (uint32_t)N_PTE; ++i) {
		pte_t entry = *ptr++;

		// only process this entry if it's not empty
		if (entry) {
			if (dir) {
				// this is a PDIR entry; could be either a 4MB
				// page, or a PMT pointer
				if (mode > Simple) {
					pde_prt(level, i, entry, false);
					if (!IS_LARGE(entry) && mode > OneLevel) {
						ptdump(level + 1, (void *)P2V(PTE_ADDR(entry)), false,
							   mode);
					}
				}
			} else {
				// just a PMT entry
				if (mode > Simple) {
					pte_prt(level, i, entry, false);
				}
			}
		}
	}
}

/**
** Name:	pmt_dump
**
** Dump the non-zero entries of a page table or directory
**
** @param pt     The page table
** @param dir    Is this a page directory?
** @param start  First entry to process
** @param num    Number of entries to process
*/
ATTR_UNUSED
static void pmt_dump(pte_t *pt, bool_t dir, uint32_t start, uint32_t num)
{
	cio_printf("\n\nP%c dump", dir ? 'D' : 'T');
	cio_printf(" of %08x", (uint32_t)pt);
	cio_printf(" [%03x] through [%03x]\n", start, start + num - 1);

	uint_t n = 0;
	uint_t z = 0;

	for (uint_t i = 0; i < num; ++i) {
		pte_t entry = pt[start + i];
		// four entries per line
		if (n && ((n & 0x3) == 0)) {
			cio_putchar('\n');
		}
		if (IS_PRESENT(entry)) {
			cio_printf(" %03x", start + i);
			if (IS_LARGE(entry)) {
				cio_printf(" 8 %05x", GET_4MFRAME(entry) << 10);
			} else {
				cio_printf(" 4 %05x", GET_4KFRAME(entry));
			}
			++n;
		} else {
			++z;
		}
		// pause after every four lines of output
		if (n && ((n & 0xf) == 0)) {
			delay(DELAY_2_SEC);
		}
	}

	// partial line?
	if ((n & 0x3) != 0) {
		cio_putchar('\n');
	}

	if (z > 0) {
		cio_printf(" %u entries were !P\n", z);
	}

	delay(DELAY_2_SEC);
}

/*
** PUBLIC FUNCTIONS
*/

/**
** Name:	vm_init
**
** Description:  Initialize the VM module
*/
void vm_init(void)
{
#if TRACING_INIT
	cio_puts(" VM");
#endif

	// set up the kernel's 4K-page directory
	kpdir = vm_mkkvm();
	assert(kpdir != NULL);

#if TRACING_VM
	cio_printf("vm_init: kpdir %08x, adding user pages\n", kpdir);
#endif

	// add the entries for the user address space
	for (uint32_t addr = 0; addr < NUM_4MB; addr += SZ_PAGE) {
		int stat = vm_map(kpdir, (void *)addr, addr, SZ_PAGE, PTE_US | PTE_RW);
		if (stat != SUCCESS) {
			cio_printf("vm_init, map %08x->%08x failed, status %d\n", addr,
					   addr, stat);
			PANIC(0, "vm_init user range map failed");
		}
#if TRACING_VM
		cio_putchar('.');
#endif
	}
#if TRACING_VM
	cio_puts(" done\n");
#endif

	// switch to it
	vm_set_kvm();

#if TRACING_VM
	cio_puts("vm_init: running on new kpdir\n");
#endif

	// install the page fault handler
	install_isr(VEC_PAGE_FAULT, vm_isr);
}

/**
** Name:    vm_uva2kva
**
** Convert a user VA into a kernel address. Works for all addresses -
** if the address is a page address, the low-order nine bits will be
** zeroes; otherwise, they are the offset into the page, which is
** unchanged between the address spaces.
**
** @param pdir  Pointer to the page directory to examine
** @param va    Virtual address to check
*/
void *vm_uva2kva(pde_t *pdir, void *va)
{
	// find the PMT entry for this address
	pte_t *pte = vm_getpte(pdir, va, false);
	if (pte == NULL) {
		return NULL;
	}

	// get the entry
	pte_t entry = *pte;

	// is this a valid address for the user?
	if (!IS_PRESENT(entry)) {
		return NULL;
	}

	// is this a system-only page?
	if (IS_SYSTEM(entry)) {
		return NULL;
	}

	// get the physical address
	uint32_t frame = PTE_ADDR(*pte) | PERMS(va);

	return (void *)P2V(frame);
}

/**
** Name:	vm_pagedup
**
** Duplicate a page of memory
**
** @param old  Pointer to the first byte of a page
**
** @return a pointer to the new, duplicate page, or NULL
*/
void *vm_pagedup(void *old)
{
	void *new = (void *)km_page_alloc();
	if (new != NULL) {
		blkmov(new, old, SZ_PAGE);
	}
	return new;
}

/**
** Name:	vm_pdedup
**
** Duplicate a page directory entry
**
** @param entry The entry to be duplicated
**
** @return the new entry, or -1 on error
*/
pde_t vm_pdedup(pde_t entry)
{
#if TRACING_VM
	cio_printf("vm_pdedup curr %08x\n", (uint32_t)entry);
#endif

	// simplest case
	if (!IS_PRESENT(entry)) {
		return 0;
	}

	// is this a large page?
	if (IS_LARGE(entry)) {
		// just copy it
		return entry;
	}

	// OK, we have a 4KB entry; allocate a page table for it
	pte_t *tblva = (pte_t *)km_page_alloc();
	if (tblva == NULL) {
		return (uint32_t)-1;
	}

	// make sure the entries are all initially 'not present'
	memclr(tblva, SZ_PAGE);

	// VA of the page table for this directory entry
	pte_t *old = (pte_t *)P2V(PDE_ADDR(entry));

	// pointer to the first PTE in the new table (already a VA)
	pte_t *new = tblva;

	for (int i = 0; i < N_PTE; ++i) {
		// only need to copy 'present' entries
		if (IS_PRESENT(*old)) {
			*new = *old;
		}
		++old;
		++new;
	}

	// replace the page table address
	// (PA of page table, lower 12 bits from '*curr')
	return (pde_t)(V2P(PTE_ADDR(tblva)) | PERMS(entry));
}

/**
** Name:	vm_getpte
**
** Return the address of the PTE corresponding to the virtual address
** 'va' within the address space controlled by 'pgdir'. If there is no
** page table for that VA and 'alloc' is true, create the necessary
** page table entries.
**
** @param pdir   Pointer to the page directory to be searched
** @param va     The virtual address we're looking for
** @param alloc  Should we allocate a page table if there isn't one?
**
** @return A pointer to the page table entry for this VA, or NULL if
**         there isn't one and we're not allocating
*/
pte_t *vm_getpte(pde_t *pdir, const void *va, bool_t alloc)
{
	pte_t *ptbl;

	// sanity check
	assert1(pdir != NULL);

	// get the PDIR entry for this virtual address
	pde_t *pde_ptr = &pdir[PDIX(va)];

	// is it already set up?
	if (IS_PRESENT(*pde_ptr)) {
		// yes!
		ptbl = (pte_t *)P2V(PTE_ADDR(*pde_ptr));

	} else {
		// no - should we create it?
		if (!alloc) {
			// nope, so just return
			return NULL;
		}

		// yes - try to allocate a page table
		ptbl = (pte_t *)km_page_alloc();
		if (ptbl == NULL) {
			WARNING("can't allocate page table");
			return NULL;
		}

		// who knows what was left in this page....
		memclr(ptbl, SZ_PAGE);

		// add this to the page directory
		//
		// we set this up to allow general access; this could be
		// controlled by setting access control in the page table
		// entries, if necessary.
		//
		// NOTE: the allocator is serving us virtual page addresses,
		// so we must convert them to physical addresses for the
		// table entries
		*pde_ptr = V2P(ptbl) | PDE_P | PDE_RW | PDE_US;
	}

	// finally, return a pointer to the entry in the page table for this VA
	return &ptbl[PTIX(va)];
}

// Set up kernel part of a page table.
pde_t *vm_mkkvm(void)
{
	mapping_t *k;

	// allocate the page directory
	pde_t *pdir = km_page_alloc();
	if (pdir == NULL) {
		return NULL;
	}
#if 0 && TRACING_VM
	cio_puts( "\nEntering vm_mkkvm\n" );
	pmt_dump( pdir, true, 0, N_PDE );
#endif

	// clear it out to disable all the entries
	memclr(pdir, SZ_PAGE);

	if (P2V(PHYS_TOP) > DEV_BASE) {
		cio_printf("PHYS_TOP (%08x -> %08x) > DEV_BASE(%08x)\n", PHYS_TOP,
				   P2V(PHYS_TOP), DEV_BASE);
		PANIC(0, "PHYS_TOP too large");
	}

	// map in all the page ranges
	k = kmap;
	for (int i = 0; i < n_kmap; ++i, ++k) {
		int stat = vm_map(pdir, ((void *)k->va_start), k->pa_start,
						  k->pa_end - k->pa_start, k->perm);
		if (stat != SUCCESS) {
			vm_free(pdir);
			return 0;
		}
	}
#if 0 && TRACING_VM
	cio_puts( "\nvm_mkkvm() final PD:\n" );
	pmt_dump( pdir, true, 0, 16 );
	pmt_dump( pdir, true, 0x200, 16 );
#endif

	return pdir;
}

/*
** Creates an initial user VM table hierarchy by copying the
** system entries into a new page directory.
**
** @return a pointer to the new page directory, or NULL
*/
pde_t *vm_mkuvm(void)
{
	// allocate the directory
	pde_t *new = (pde_t *)km_page_alloc();
	if (new == NULL) {
		return NULL;
	}

	// iterate through the 'system' portions of the kernel
	// page directory
	int i = PDIX(KERN_BASE);
	pde_t *curr = &kpdir[i];
	pde_t *dst = &new[i];
	while (i < N_PDE) {
		if (*curr != 0) {
			// found an active one - duplicate it
			pde_t entry = vm_pdedup(*curr);
			if (entry == (uint32_t)-1) {
				return NULL;
			}
			*dst = entry;
		} else {
			*dst = 0;
		}

		++curr;
		++dst;
		++i;
	}

	return new;
}

/**
** Name:	vm_set_kvm
**
** Switch the page table register to the kernel's page directory.
*/
void vm_set_kvm(void)
{
#if TRACING_VM
	cio_puts("Entering vm_set_kvm()\n");
#endif
	w_cr3(V2P(kpdir)); // switch to the kernel page table
#if TRACING_VM
	cio_puts("Exiting vm_set_kvm()\n");
#endif
}

/**
** Name:	vm_set_uvm
**
** Switch the page table register to the page directory for a user process.
**
** @param p  PCB of the process we're switching to
*/
void vm_set_uvm(pcb_t *p)
{
#if TRACING_VM
	cio_puts("Entering vm_set_uvm()\n");
#endif
	assert(p != NULL);
	assert(p->pdir != NULL);

	w_cr3(V2P(p->pdir)); // switch to process's address space
#if TRACING_VM
	cio_puts("Entering vm_set_uvm()\n");
#endif
}

/**
** Name:	vm_add
**
** Add pages to the page hierarchy for a process, copying data into
** them if necessary.
**
** @param pdir   Pointer to the page directory to modify
** @param wr     "Writable" flag for the PTE
** @param sys    "System" flag for the PTE
** @param va     Starting VA of the range
** @param size   Amount of physical memory to allocate (bytes)
** @param data   Pointer to data to copy, or NULL
** @param bytes  Number of bytes to copy
**
** @return status of the allocation attempt
*/
int vm_add(pde_t *pdir, bool_t wr, bool_t sys, void *va, uint32_t size,
		   char *data, uint32_t bytes)
{
	// how many pages do we need?
	uint32_t npages = ((size & MOD4K_BITS) ? PGUP(size) : size) >> MOD4K_SHIFT;

	// permission set for the PTEs
	uint32_t entrybase = PTE_P;
	if (wr) {
		entrybase |= PTE_RW;
	}
	if (!sys) {
		entrybase |= PTE_US;
	}

#if TRACING_VM
	cio_printf("vm_add: pdir %08x, %s, va %08x size %u (%u pgs)\n",
			   (uint32_t)pdir, wr ? "W" : "!W", (uint32_t)va, size, npages);
	cio_printf("        from %08x, %u bytes, perms %08x\n", (uint32_t)data,
			   bytes, entrybase);
#endif

	// iterate through the pages

	for (int i = 0; i < npages; ++i) {
		// figure out where this page will go in the hierarchy
		pte_t *pte = vm_getpte(pdir, va, true);
		if (pte == NULL) {
			// if i > 0, this isn't the first frame - is
			// there anything to do about other frames?
			// POSSIBLE MEMORY LEAK?
			return E_NO_MEMORY;
		}

		// allocate the frame
		void *page = km_page_alloc();
		if (page == NULL) {
			// same question here
			return E_NO_MEMORY;
		}

		// clear it all out
		memclr(page, SZ_PAGE);

		// create the PTE for this frame
		uint32_t entry = (uint32_t)(V2P(PTE_ADDR(page)) | entrybase);
		*pte = entry;

		// copy data if we need to
		if (data != NULL && bytes > 0) {
			// how much to copy
			uint32_t num = bytes > SZ_PAGE ? SZ_PAGE : bytes;
			// do it!
			memmove((void *)page, (void *)data, num);
			// adjust all the pointers
			data += num; // where to continue
			bytes -= num; // what's left to copy
		}

		// bump the virtual address
		va += SZ_PAGE;
	}

	return SUCCESS;
}

/**
** Name:    vm_free
**
** Deallocate a page table hierarchy and all physical memory frames
** in the user portion.
**
** Works only for 4KB pages.
**
** @param pdir  Pointer to the page directory
*/
void vm_free(pde_t *pdir)
{
#if TRACING_VM
	cio_printf("vm_free(%08x)\n", (uint32_t)pdir);
#endif

	// do we have anything to do?
	if (pdir == NULL) {
		return;
	}

	// iterate through the page directory entries, freeing the
	// PMTS and the frames they point to
	pde_t *curr = pdir;
	int nf = 0;
	int nt = 0;

	for (int i = 0; i < N_PDE; ++i) {
		// the entry itself
		pde_t entry = *curr;

		// does this entry point to anything useful?
		if (IS_PRESENT(entry)) {
			// yes - large pages make us unhappy
			assert(!IS_LARGE(entry));

			// get the PMT pointer
			pte_t *pmt = (pte_t *)P2V(PTE_ADDR(entry));

			// walk the PMT
			for (int j = 0; j < N_PTE; ++j) {
				pte_t tmp = *pmt;
				// does this entry point to a frame?
				if (IS_PRESENT(tmp)) {
					// yes - free the frame
					km_page_free((void *)P2V(PTE_ADDR(tmp)));
					++nf;
					// mark it so we don't get surprised
					*pmt = 0;
				}
				// move on
				++pmt;
			}
			// now, free the PMT itself
			km_page_free((void *)P2V(PDE_ADDR(entry)));
			++nt;
			*curr = 0;
		}

		// move to the next entry
		++curr;
	}

	// finally, free the PDIR itself
	km_page_free((void *)pdir);
	++nt;

#if TRACING_VM
	cio_printf("vm_free: %d pages, %d tables\n", nf, nt);
#endif
}

/*
** Name:	vm_map
**
** Create PTEs for virtual addresses starting at 'va' that refer to
** physical addresses in the range [pa, pa+size-1]. We aren't guaranteed
** that va is page-aligned.
**
** @param pdir  Page directory for this address space
** @param va    The starting virtual address
** @param pa    The starting physical address
** @param size  Length of the range to be mapped
** @param perm  Permission bits for the PTEs
**
** @return the status of the mapping attempt
*/
int vm_map(pde_t *pdir, void *va, uint32_t pa, uint32_t size, int perm)
{
	// round the VA down to its page boundary
	char *addr = (char *)PGDOWN((uint32_t)va);

	// round the end of the range down to its page boundary
	char *last = (char *)PGDOWN(((uint32_t)va) + size - 1);

#if TRACING_VM
	cio_printf("vm_map pdir %08x va %08x pa %08x size %08x perm %03x\n",
			   (uint32_t)pdir, (uint32_t)va, pa, size, perm);
#endif

	while (addr <= last) {
		// get a pointer to the PTE for the current VA
		pte_t *pte = vm_getpte(pdir, addr, true);
		if (pte == NULL) {
			// couldn't find it
			return E_NO_PTE;
		}
#if 0 && TRACING_VM
		cio_printf( "  addr %08x pa %08x last %08x pte %08x *pte %08x\n",
			(uint32_t) addr, pa, (uint32_t) last, (uint32_t) pte, *pte
		);
#endif

		// create the new entry for the page table
		pde_t newpte = pa | perm | PTE_P;

		// if this entry has already been mapped, we're in trouble
		if (IS_PRESENT(*pte)) {
			if (*pte != newpte) {
#if TRACING_VM
				cio_printf(
					"vm_map: va %08x pa %08x pte %08x *pte %08x entry %08x\n",
					(uint32_t)va, pa, (uint32_t)pte, (uint32_t)*pte, newpte);
				cio_printf(" addr %08x PDIX 0x%x PTIX 0x%x\n", (uint32_t)addr,
						   PDIX(addr), PTIX(addr));

				// dump the directory
				pmt_dump(pdir, true, PDIX(addr), 4);

				// find the relevant PDE entry
				uint32_t ix = PDIX(va);
				pde_t entry = pdir[ix];
				if (!IS_LARGE(entry)) {
					// round the PMT index down
					uint32_t ix2 = PTIX(va) & MOD4_MASK;
					// dump the PMT for the relevant directory entry
					pmt_dump((void *)P2V(PDE_ADDR(entry)), false, ix2, 4);
				}
#endif
				PANIC(0, "mapping an already-mapped address");
			}
		}

		// ok, set the PTE as requested
		*pte = newpte;

		// nope - move to the next page
		addr += SZ_PAGE;
		pa += SZ_PAGE;
	}
	return SUCCESS;
}

/**
** Name:	vm_uvmdup
**
** Create a duplicate of the user portio of an existing page table
** hierarchy. We assume that the "new" page directory exists and
** the system portions of it should not be touched.
**
** Note: we do not duplicate the frames in the hierarchy - we just
** create a duplicate of the hierarchy itself. This means that we
** now have two sets of page tables that refer to the same physical
** frames in memory.
**
** @param new  New page directory
** @param old  Existing page directory
**
** @return status of the duplication attempt
*/
int vm_uvmdup(pde_t *new, pde_t *old)
{
	if (old == NULL || new == NULL) {
		return E_BAD_PARAM;
	}

#if TRACING_VM
	cio_printf("vmdup: old %08x new %08x\n", (uint32_t)old, (uint32_t)new);
#endif

	// we only want to deal with the "user" half of the address space
	for (int i = 0; i < (N_PDE >> 1); ++i) {
		// the entry to copy
		pde_t entry = *old;

		// is this entry in use?
		if (IS_PRESENT(entry)) {
			// yes. if it points to a 4MB page, we just copy it;
			// otherwise, we must duplicate the next level PMT

			if (!IS_LARGE(entry)) {
				// it's a 4KB page, so we need to duplicate the PMT
				pte_t *newpt =
					(pte_t *)vm_pagedup((void *)P2V(PTE_ADDR(entry)));
				if (newpt == NULL) {
					return E_NO_MEMORY;
				}

				uint32_t perms = PERMS(entry);

				// create the new PDE entry by replacing the frame #
				entry = ((uint32_t)V2P(PTE_ADDR(newpt))) | perms;
			}

		} else {
			// not present, so create an empty entry
			entry = 0;
		}

		// send it on its way
		*new = entry;

		// move on down the line
		++old;
		++new;
	}

	return SUCCESS;
}

/**
** Name:	vm_print
**
** Print out a paging hierarchy.
**
** @param pt    Page table to display
** @param dir   Is it a page directory (vs. a page table)?
** @param mode  How to display the entries
*/
void vm_print(void *pt, bool_t dir, enum vmmode_e mode)
{
	cio_puts("\nVM hierarchy");
	if (pt == NULL) {
		cio_puts(" (NULL pointer)\n");
		return;
	}

	cio_printf(", starting at 0x%08x (%s):\n", (uint32_t)pt,
			   dir ? "PDIR" : "PMT");

	ptdump(1, pt, dir, mode);
}
