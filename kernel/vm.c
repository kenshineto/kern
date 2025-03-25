/**
** @file	vm.c
**
** @author	CSCI-452 class of 20245
**
** @brief	Kernel VM support
*/

#define	KERNEL_SRC

#include <common.h>

#include <vm.h>
#include <vmtables.h>

#include <kmem.h>
#include <procs.h>
#include <x86/arch.h>
#include <x86/ops.h>

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
static void vm_isr( int vector, int code ) {

	// get whatever information we can from the fault
	pfec_t fault;
	fault.u = (uint32_t) code;
	uint32_t addr = r_cr2();

	// report what we found
	sprint( b256,
		"** page fault @ 0x%08x %cP %c %cM %cRSV %c %cPK %cSS %cHLAT %cSGZ",
			addr,
			fault.s.p    ? ' ' : '!',
			fault.s.w    ? 'W' : 'R',
			fault.s.us   ? 'U' : 'S',
			fault.s.rsvd ? ' ' : '!',
			fault.s.id   ? 'I' : 'D',
			fault.s.pk   ? ' ' : '!',
			fault.s.ss   ? ' ' : '!',
			fault.s.hlat ? ' ' : '!',
			fault.s.sgz  ? ' ' : '!'
	);

	// and give up
	PANIC( 0, b256 );
}

/**
** Name:    uva2kva
**
** Convert a user VA into a kernel address
*/
__attribute__((__unused__))
static void *uva2kva( pde_t *pdir, void *va ) {

	// find the PMT entry for this address
	pte_t *pte = vm_getpte( pdir, va, false );
	if( pte == NULL ) {
		return NULL;
	}

	// is this a valid address for the user?
	if( IS_PRESENT(*pte) ) {
		return 0;
	}

	if( IS_LARGE(*pte) ) {
		return 0;
	}

	// get the physical address
	uint32_t frame = *pte & FRAME_MASK;     // keep the frame address
	frame |= ((uint32_t) va) & PERM_MASK;  // OR in the lower 12 bits

	return (void *) frame;
}


/*
** PUBLIC FUNCTIONS
*/

/**
** Name:	vm_init
**
** Description:  Initialize the VM module
*/
void vm_init( void ) {

#if TRACING_INIT
	cio_puts( " VM" );
#endif

	// set up the kernel's page directory
	kpdir = vm_mkkvm();
	assert( kpdir != NULL );

	// install the page fault handler
	install_isr( VEC_PAGE_FAULT, vm_isr );
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
void *vm_pagedup( void *old ) {
	void *new = (void *) km_page_alloc();
	if( new != NULL ) {
		memcpy( new, old, SZ_PAGE );
	}
	return new;
}

/**
** Name:	vm_ptdup
**
** Duplicate a page directory entry
**
** @param dst   Pointer to where the duplicate should go
** @param curr  Pointer to the entry to be duplicated
**
** @return true on success, else false
*/
bool_t vm_ptdup( pde_t *dst, pde_t *curr ) {

#if TRACING_VM
	cio_printf( "vm_ptdup dst %08x curr %08x\n",
			(uint32_t) dst, (uint32_t) curr );
#endif
	// simplest case
	if( *curr == 0 ) {
		*dst = 0;
		return true;
	}

	// OK, we have an entry; allocate a page table
	pte_t *pt = (pte_t *) km_page_alloc();
	if( pt == NULL ) {
		return false;
	}

	// pointer to the first PTE in the current table
	pte_t *old = (pte_t *) (((uint32_t) *curr) & FRAME_MASK);
	// pointer to the first PTE in the new table
	pte_t *new = pt;

	for( int i = 0 ; i < N_PTE; ++i ) {
		if( IS_PRESENT(*old) ) {
			*new = 0;
		} else {
			*new = *old;
		}
		++old;
		++new;
	}

	// assign the page table into the new page directory
	// upper 22 bits from 'pt', lower 12 from '*curr'
	*dst = (pde_t) (
		(((uint32_t)pt) & FRAME_MASK) |
		(((uint32_t)(*curr)) & PERM_MASK )
	);

	return true;
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
** @return A pointer to the page table entry for this VA, or NULL
*/
pte_t *vm_getpte( pde_t *pdir, const void *va, bool_t alloc ) {
	pte_t *ptab;

	// sanity check
	assert1( pdir != NULL );

	// get the PDIR entry for this virtual address
	pde_t *pde = &pdir[ PDIX(va) ];

	// is it already set up?
	if( IS_PRESENT(*pde) ) {

		// yes!
		ptab = (pte_t*)P2V(PTE_ADDR(*pde));

	} else {

		// no - should we create it?
		if( !alloc ) {
			// nope, so just return
			return NULL;
		}

		// yes - try to allocate a page table
		ptab = (pte_t *) km_page_alloc();
		if( ptab == NULL ) {
			WARNING( "can't allocate page table" );
			return NULL;
		}

		// who knows what was left in this page....
		memclr( ptab, SZ_PAGE );

		// add this to the page directory
		//
		// we set this up to allow general access; this could be
		// controlled by setting access control in the page table
		// entries, if necessary.
		*pde = V2P(ptab) | PDE_P | PDE_RW;
	}

	// finally, return a pointer to the entry in the
	// page table for this VA
	return &ptab[ PTIX(va) ];
}

// Set up kernel part of a page table.
pde_t *vm_mkkvm( void )
{
	mapping_t *k;

	// allocate the page directory
	pde_t *pdir = km_page_alloc();
	if( pdir == NULL ) {
		return NULL;
	}

	// clear it out to disable all the entries
	memclr( pdir, SZ_PAGE );

	// map in all the page ranges
	k = kmap;
	for( int i = 0; i < n_kmap; ++i, ++k ) {
		int stat = vm_map( pdir, ((void *)k->va_start),
				k->pa_end - k->pa_start, 
				k->pa_start, k->perm );
		if( stat != SUCCESS ) {
			vm_free( pdir );
			return 0;
		}
	}

	return pdir;
}

/*
** Creates an initial user VM table hierarchy by copying the
** system entries into a new page directory.
**
** @return a pointer to the new page directory, or NULL
*/
pde_t *vm_mkuvm( void ) {

	// allocate the directory
	pde_t *new = (pde_t *) km_page_alloc();
	if( new == NULL ) {
		return NULL;
	}

	// iterate through the kernel page directory
	pde_t *curr = kpdir;
	pde_t *dst = new;
	for( int i = 0; i < N_PDE; ++i ) {

		if( *curr != 0 ) {
			// found an active one - duplicate it
			if( !vm_ptdup(dst,curr) ) {
				return NULL;
			}
		}

		++curr;
		++dst;
	}

	return new;

}

/**
** Name:	vm_set_kvm
**
** Switch the page table register to the kernel's page directory.
*/
void vm_set_kvm( void ) {
	w_cr3( V2P(kpdir) );   // switch to the kernel page table
}

/**
** Name:	vm_set_uvm
**
** Switch the page table register to the page directory for a user process.
**
** @param p  PCB of the process we're switching to
*/
void vm_set_uvm( pcb_t *p ) {
	assert( p != NULL );
	assert( p->pdir != NULL );

	w_cr3( V2P(p->pdir) );  // switch to process's address space
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
int vm_add( pde_t *pdir, bool_t wr, bool_t sys,
		void *va, uint32_t size, char *data, uint32_t bytes ) {

	// how many pages do we need?
	uint_t npages = ((size & MOD4K_BITS) ? PGUP(size) : size) >> MOD4K_SHIFT;

	// permission set for the PTEs
	uint_t entrybase = PTE_P;
	if( wr ) {
		entrybase |= PTE_RW;
	}
	if( sys ) {
		entrybase |= PTE_US;
	}

#if TRACING_VM
	cio_printf( "vm_add: pdir %08x, %s, va %08x (%u, %u pgs)\n",
			(uint32_t) pdir, wr ? "W" : "!W", (uint32_t) va, size );
	cio_printf( "        from %08x, %u bytes, perms %08x\n",
			(uint32_t) data, bytes, entrybase );
#endif

	// iterate through the pages

	for( int i = 0; i < npages; ++i ) {

		// figure out where this page will go in the hierarchy
		pte_t *pte = vm_getpte( pdir, va, true );
		if( pte == NULL ) {
			// TODO if i > 0, this isn't the first frame - is
			// there anything to do about other frames?
			// POSSIBLE MEMORY LEAK?
			return E_NO_MEMORY;
		}

		// allocate the frame
		void *page = km_page_alloc();
		if( page == NULL ) {
			// TODO same question here
			return E_NO_MEMORY;
		}

		// clear it all out
		memclr( page, SZ_PAGE );

		// create the PTE for this frame
		uint32_t entry = (uint32_t) (PTE_ADDR(page) | entrybase);
		*pte = entry;

		// copy data if we need to
		if( data != NULL && bytes > 0 ) {
			// how much to copy
			uint_t num = bytes > SZ_PAGE ? SZ_PAGE : bytes;
			// do it!
			memcpy( (void *)page, (void *)data, num );
			// adjust all the pointers
			data += num;   // where to continue
			bytes -= num;  // what's left to copy
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
** @param pdir  Pointer to the page directory
*/
void vm_free( pde_t *pdir ) {

	// do we have anything to do?
	if( pdir == NULL ) {
		return;
	}

	// iterate through the page directory entries, freeing the
	// PMTS and the frames they point to
	pde_t *curr = pdir;
	for( int i = 0; i < N_PDE; ++i ) {

		// does this entry point to anything useful?
		if( IS_PRESENT(*curr) ) {

			// yes - get the PMT pointer
			pte_t *pte = (pte_t *) PTE_ADDR(*curr);

			// walk the PMT
			for( int j = 0; j < N_PTE; ++j ) {
				// does this entry point to a frame?
				if( IS_PRESENT(*pte) ) {
					// yes - free the frame
					km_page_free( (void *) PTE_ADDR(*pte) );
					// mark it so we don't get surprised
					*pte = 0;
				}
				// move on
				++pte;
			}
			// now, free the PMT itself
			km_page_free( (void *) PDE_ADDR(*curr) );
			*curr = 0;
		}

		// move to the next entry
		++curr;
	}

	// finally, free the PDIR itself
	km_page_free( (void *) pdir );
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
** @param size  Length of the range to be mapped
** @param pa    The starting physical address
** @param perm  Permission bits for the PTEs
*/
int vm_map( pde_t *pdir, void *va, uint_t size, uint_t pa, int perm ) {
	pte_t *pte;

	// round the VA down to its page boundary
	char *addr = (char*)PGDOWN((uint_t)va);

	// round the end of the range down to its page boundary
	char *last = (char*)PGDOWN(((uint_t)va) + size - 1);

	for(;;) {

		// get a pointer to the PTE for the current VA
		if( (pte = vm_getpte(pdir, addr, 1)) == 0 ) {
			// couldn't find it
			return E_NO_PTE;
		}

		// if this entry has already been mapped, we're in trouble
		if( IS_PRESENT(*pte) ) {
			PANIC( 0, "mapping an already-mapped address" );
		}

		// ok, set the PTE as requested
		*pte = pa | perm | PTE_P;

		// are we done?
		if( addr == last ) {
			break;
		}

		// nope - move to the next page
		addr += SZ_PAGE;
		pa += SZ_PAGE;
	}
	return 0;
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
** now have two sets of page tables that refer to the same user-level
** frames in memory.
**
** @param old  Existing page directory
** @param new  New page directory
**
** @return status of the duplication attempt
*/
int vm_uvmdup( pde_t *old, pde_t *new ) {

	if( old == NULL || new == NULL ) {
		return E_BAD_PARAM;
	}

	// we only want to deal with the "user" half of the address space
	for( int i = 0; i < (N_PDE >> 1); ++i ) {

		// is this entry in use?
		if( IS_PRESENT(*old) ) {

			// yes. if it points to a 4MB page, we just copy it;
			// otherwise, we must duplicate the next level PMT

			*new = *old;  // copy the entry

			if( !IS_LARGE(*old) ) {

				// it's a 4KB page, so we need to duplicate the PMT
				pte_t *newpmt = (pte_t *) vm_pagedup( (void *) (*old & FRAME_MASK) );
				if( newpmt == NULL ) {
					return E_NO_MEMORY;
				}

				// create the new PDE entry by replacing the frame #
				*new = (pde_t) (((uint32_t)newpmt) | PERMS(*old));
			}
		}

		++old;
		++new;
	}

	return SUCCESS;
}
