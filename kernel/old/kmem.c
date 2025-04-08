/**
** @file kmem.c
**
** @author Warren R. Carithers
** @author Kenneth Reek
** @author 4003-506 class of 20013
**
** @brief	Functions to perform dynamic memory allocation in the OS.
**
** NOTE: these should NOT be called by user processes!
**
** This allocator functions as a simple "slab" allocator; it allows
** allocation of either 4096-byte ("page") or 1024-byte ("slice")
** chunks of memory from the free pool.  The free pool is initialized
** using the memory map provided by the BIOS during the boot sequence,
** and contains a series of blocks which are each one page of memory
** (4KB, and aligned at 4KB boundaries); they are held in the free list
** in LIFO order, as all pages are created equal.
**
** By default, the addresses used are virtual addresses rather than
** physical addresses. Define the symbol USE_PADDRS when compiling to
** change this.
**
** Each allocator ("page" and "slice") allocates the first block from
** the appropriate free list.  On deallocation, the block is added back
** to the free list.
**
** The "slice" allocator operates by taking blocks from the "page"
** allocator and splitting them into four 1K slices, which it then
** manages.  Requests are made for slices one at a time.  If the free
** list contains an available slice, it is unlinked and returned;
** otherwise, a page is requested from the page allocator, split into
** slices, and the slices are added to the free list, after which the
** first one is returned.  The slice free list is a simple linked list
** of these 1K blocks; because they are all the same size, no ordering
** is done on the free list, and no coalescing is performed.
**
** This could be converted into a bitmap-based allocator pretty easily.
** A 4GB address space contains 2^20 (1,048,576) pages; at one bit per
** page frame, that's 131,072 (2^17) bytes to cover all of the address
** space, and that could be reduced by restricting allocatable space
** to a subset of the 4GB space.
**
** Compilation options:
**
**    ALLOC_FAIL_PANIC    if an internal slice allocation fails, panic
**    USE_PADDRS          build the free list using physical, not
**                        virtual, addresses
*/

#define KERNEL_SRC

#include <common.h>

// all other framework includes are next
#include <lib.h>

#include <kmem.h>

#include <list.h>
#include <x86/arch.h>
#include <x86/bios.h>
#include <bootstrap.h>
#include <cio.h>
#include <vm.h>

/*
** PRIVATE DEFINITIONS
*/

// combination tracing tests
#define ANY_KMEM       (TRACING_KMEM|TRACING_KMEM_INIT|TRACING_KMEM_FREELIST)
#define KMEM_OR_INIT   (TRACING_KMEM|TRACING_KMEM_INIT)

// parameters related to word and block sizes

#define WORD_SIZE           sizeof(int)
#define LOG2_OF_WORD_SIZE   2

#define LOG2_OF_PAGE_SIZE   12

#define LOG2_OF_SLICE_SIZE  10

// converters:  pages to bytes, bytes to pages

#define P2B(x)   ((x) << LOG2_OF_PAGE_SIZE)
#define B2P(x)   ((x) >> LOG2_OF_PAGE_SIZE)

/*
** Name:    adjacent
**
** Arguments:   addresses of two blocks
**
** Description: Determines whether the second block immediately
**      follows the first one.
*/
#define adjacent(first,second)  \
	( (void *) (first) + P2B((first)->pages) == (void *) (second) )

/*
** PRIVATE DATA TYPES
*/

/*
** Memory region information returned by the BIOS
**
** This data consists of a 32-bit integer followed
** by an array of region descriptor structures.
*/

// a handy union for playing with 64-bit addresses
typedef union b64_u {
	uint32_t part[2];
	uint64_t all;
} b64_t;

// the halves of a 64-bit address
#define LOW		part[0]
#define HIGH	part[1]

// memory region descriptor
typedef struct memregion_s {
	b64_t	 base;		// base address
	b64_t	 length;	// region length
	uint32_t type;		// type of region
	uint32_t acpi;		// ACPI 3.0 info
} ATTR_PACKED region_t;

/*
** Region types
*/

#define REGION_USABLE		1
#define REGION_RESERVED		2
#define REGION_ACPI_RECL	3
#define REGION_ACPI_NVS		4
#define REGION_BAD			5

/*
** ACPI 3.0 bit fields
*/

#define REGION_IGNORE		0x01
#define REGION_NONVOL		0x02

/*
** 32-bit and 64-bit address values as 64-bit literals
*/

#define ADDR_BIT_32		0x0000000100000000LL
#define ADDR_LOW_HALF	0x00000000ffffffffLL
#define ADDR_HIGH_HALR	0xffffffff00000000LL

#define ADDR_32_MAX		ADDR_LOW_HALF
#define ADDR_64_FIRST	ADDR_BIT_32

/*
** PRIVATE GLOBAL VARIABLES
*/

// freespace pools
static list_t free_pages;
static list_t free_slices;

// block counts
static uint32_t n_pages;
static uint32_t n_slices;

// initialization status
static int km_initialized;

/*
** IMPORTED GLOBAL VARIABLES
*/

// this is no longer used; for simple situations, it can be used as
// the KM_LOW_CUTOFF value
//
// extern int _end;	// end of the BSS section - provided by the linker

/*
** FUNCTIONS
*/

/*
** FREE LIST MANAGEMENT
*/

/**
** Name:	add_block
**
** Add a block to the free list. The block will be split into separate
** page-sized fragments which will each be added to the free_pages
** list; each of these will also be modified.
**
** @param[in] base   Base physical address of the block
** @param[in] length Block length, in bytes
*/
static void add_block( uint32_t base, uint32_t length ) {

	// don't add it if it isn't at least 4K
	if( length < SZ_PAGE ) {
		return;
	}

#if ANY_KMEM
	cio_printf( "  add(%08x,%08x): ", base, length );
#endif

	// verify that the base address is a 4K boundary
	if( (base & MOD4K_BITS) != 0 ) {
		// nope - how many bytes will we lose from the beginning
		uint_t loss = base & MOD4K_BITS;
		// adjust the starting address: (n + 4K - 1) / 4K
		base = (base + MOD4K_BITS) & MOD4K_MASK;
		// adjust the length
		length -= loss;
	}

	// only want to add multiples of 4K; check the lower bits
	if( (length & MOD4K_BITS) != 0 ) {
		// round it down to 4K
		length &= MOD4K_MASK;
	}

	// determine the starting and ending addresses for the block
#ifndef USE_PADDRS
	// starting and ending addresses as virtual addresses
	base = P2V(base);
#endif
	// endpoint of the block
	uint32_t blend = base + length;

	// page count for this block
	int npages = 0;

#if ANY_KMEM
	cio_printf( "-> base %08x len %08x: ", base, length );
#endif

	// iterate through the block page by page
	while( base < blend ) {
		list_add( &free_pages, (void *) base );
		++npages;
		base  += SZ_PAGE;
	}

	// add the count to our running total
	n_pages += npages;

#if ANY_KMEM
	cio_printf( " -> %d pages\n", npages );
#endif
}

/**
** Name:	km_init
**
** Find what memory is present on the system and
** construct the list of free memory blocks.
**
** Dependencies:
**	Must be called before any other init routine that uses
**	dynamic storage is called.
*/
void km_init( void ) {
	int32_t entries;
	region_t *region;

#if TRACING_INIT
	// announce that we're starting initialization
	cio_puts( " Kmem" );
#endif

	// initially, nothing in the free lists
	free_slices.next = NULL;
	free_pages.next = NULL;
	n_pages = n_slices = 0;
	km_initialized = 0;

	// get the list length
	entries = *((int32_t *) MMAP_ADDR);

#if KMEM_OR_INIT
	cio_printf( "\nKmem: %d regions\n", entries );
#endif

	// if there are no entries, we have nothing to do!
	if( entries < 1 ) {  // note: entries == -1 could occur!
		return;
	}

	// iterate through the entries, adding things to the freelist

	region = ((region_t *) (MMAP_ADDR + 4));

	for( int i = 0; i < entries; ++i, ++region ) {

#if KMEM_OR_INIT
		// report this region
		cio_printf( "%3d: ", i );
		cio_printf( " B %08x%08x",
				region->base.HIGH, region->base.LOW );
		cio_printf( " L %08x%08x",
				region->length.HIGH, region->length.LOW );
		cio_printf( " T %08x A %08x",
				region->type, region->acpi );
#endif

		/*
		** Determine whether or not we should ignore this region.
		**
		** We ignore regions for several reasons:
		**
		**  ACPI indicates it should be ignored
		**  ACPI indicates it's non-volatile memory
		**  Region type isn't "usable"
		**  Region is above our address limit
		**
		** Currently, only "normal" (type 1) regions are considered
		** "usable" for our purposes.  We could potentially expand
		** this to include ACPI "reclaimable" memory.
		*/

		// first, check the ACPI one-bit flags

		if( ((region->acpi) & REGION_IGNORE) == 0 ) {
#if KMEM_OR_INIT
			cio_puts( " IGN\n" );
#endif
			continue;
		}

		if( ((region->acpi) & REGION_NONVOL) != 0 ) {
#if KMEM_OR_INIT
			cio_puts( " NVOL\n" );
#endif
			continue;  // we'll ignore this, too
		}

		// next, the region type

		if( (region->type) != REGION_USABLE ) {
#if KMEM_OR_INIT
			cio_puts( " RCLM\n" );
#endif
			continue;  // we won't attempt to reclaim ACPI memory (yet)
		}

		/*
		** We have a "normal" memory region. We need to verify
		** that it's within our constraints.
		**
		** We ignore anything below our KM_LOW_CUTOFF address. (In theory,
		** we should be able to re-use much of that space; in practice,
		** this is safer.) We won't add anything to the free list if it is:
		**
		**    * below our KM_LOW_CUTOFF value
		**    * above out KM_HIGH_CUTOFF value.
		**
		** For blocks which straddle one of those limits, we will
		** split it, and only use the portion that's within those
		** bounds.
		*/

		// grab the two 64-bit values to simplify things
		uint64_t base   = region->base.all;
		uint64_t length = region->length.all;
		uint64_t endpt  = base + length;

		// ignore it if it's above our high cutoff point
		if( base >= KM_HIGH_CUTOFF || endpt >= KM_HIGH_CUTOFF ) {

			// is the whole thing too high, or just part?
			if( base >= KM_HIGH_CUTOFF ) {
				// it's all too high!
#if KMEM_OR_INIT
				cio_puts( " HIGH\n" );
#endif
				continue;
			}

			// some of it is usable - fix the end point
			endpt = KM_HIGH_CUTOFF;
		}

		// see if it's below our low cutoff point
		if( base < KM_LOW_CUTOFF || endpt < KM_LOW_CUTOFF ) {

			// is the whole thing too low, or just part?
			if( endpt < KM_LOW_CUTOFF ) {
				// it's all below the cutoff!
#if KMEM_OR_INIT
				cio_puts( " LOW\n" );
#endif
				continue;
			}

			// some of it is usable - reset the base address
			base = KM_LOW_CUTOFF;

			// recalculate the length
			length = endpt - base;
		}

		// we survived the gauntlet - add the new block
		//
		// we may have changed the base or endpoint, so
		// we should recalculate the length
		length = endpt - base;

#if KMEM_OR_INIT
		cio_puts( " OK\n" );
#endif

		uint32_t b32 = base   & ADDR_LOW_HALF;
		uint32_t l32 = length & ADDR_LOW_HALF;

		add_block( b32, l32 );
	}

	// record the initialization
	km_initialized = 1;

#if KMEM_OR_INIT
	delay( DELAY_3_SEC );
#endif
}

/**
** Name:	km_dump
**
** Dump information about the free lists to the console. By default,
** prints only the list sizes; if 'addrs' is true, also dumps the list
** of page addresses; if 'all' is also true, dumps page addresses and
** slice addresses.
**
** @param addrs  Also dump page addresses
** @param both   Also dump slice addresses
*/
void km_dump( bool_t addrs, bool_t both ) {

	// report the sizes
	cio_printf( "&free_pages %08x, &free_slices %08x, %u pages, %u slices\n",
			(uint32_t) &free_pages, (uint32_t) &free_slices,
			n_pages, n_slices );

	// was that all?
	if( !addrs ) {
		return;
	}

	// dump the addresses of the pages in the free list
	uint32_t n = 0;
	list_t *block = free_pages.next;
	while( block != NULL ) {
		if( n && !(n & MOD4_BITS) ) {
			// four per line
			cio_putchar( '\n' );
		}
		cio_printf( " page @ 0x%08x", (uint32_t) block );
		block = block->next;
		++n;
	}

	// sanity check - verify that the counts match
	if( n != n_pages ) {
		sprint( b256, "km_dump: n_pages %u, counted %u!!!\n",
				n_pages, n );
		WARNING( b256);
	}

	if( !both ) {
		return;
	}

	// but wait - there's more!

	// also dump the addresses of slices in the slice free list
	n = 0;
	block = free_slices.next;
	while( block != NULL ) {
		if( n && !(n & MOD4_BITS) ) {
			// four per line
			cio_putchar( '\n' );
		}
		cio_printf( "  slc @ 0x%08x", (uint32_t) block );
		block = block->next;
		++n;
	}

	// sanity check - verify that the counts match
	if( n != n_slices ) {
		sprint( b256, "km_dump: n_slices %u, counted %u!!!\n",
				n_slices, n );
		WARNING( b256);
	}
}

/*
** PAGE MANAGEMENT
*/

/**
** Name:	km_page_alloc
**
** Allocate a page of memory from the free list.
**
** @return a pointer to the beginning of the allocated page,
**		   or NULL if no memory is available
*/
void *km_page_alloc( void ) {

	// if km_init() wasn't called first, stop us in our tracks
	assert( km_initialized );

#if TRACING_KMEM_FREELIST
	cio_puts( "KM: pg_alloc()" );
#endif

	// pointer to the first block
	void *page = list_remove( &free_pages );

	// was a page available?
	if( page == NULL ){
		// nope!
#if TRACING_KMEM_FREELIST
		cio_puts( " FAIL\n" );
#endif
#if ALLOC_FAIL_PANIC
		PANIC( 0, "page alloc failed" );
#else
		return NULL;
#endif
	}

	// fix the count of available pages
	--n_pages;

#if TRACING_KMEM_FREELIST
	cio_printf( " -> %08x\n", (uint32_t) page );
#endif

	return( page );
}

/**
** Name:	km_page_free
**
** Returns a page to the list of available pages.
**
** @param[in] page   Pointer to the page to be returned to the free list
*/
void km_page_free( void *page ){

	// verify that km_init() was called first
	assert( km_initialized );

#if TRACING_KMEM_FREELIST
	cio_printf( "KM: pg_free(%08x)\n", (uint32_t) page );
#endif

	/*
	** Don't do anything if the address is NULL.
	*/
	if( page == NULL ){
		return;
	}


	/*
	** CRITICAL ASSUMPTION
	**
	** We assume that the block pointer given to us points to a single
	** page-sized block of memory.  We make this assumption because we
	** don't track allocation sizes.  We can't use the simple "allocate
	** four extra bytes before the returned pointer" scheme to do this
	** because we're managing pages, and the pointers we return must point
	** to page boundaries, so we would wind up allocating an extra page
	** for each allocation.
	**
	** Alternatively, we could keep an array of addresses and block
	** sizes ourselves, but that feels clunky, and would risk running out
	** of table entries if there are lots of allocations (assuming we use
	** a 4KB page to hold the table, at eight bytes per entry we would have
	** 512 entries per page).
	**
	** IF THIS ASSUMPTION CHANGES, THIS CODE MUST BE FIXED!!!
	*/

	// link this into the free list
	list_add( &free_pages, page );

	// one more in the pool
	++n_pages;
}

/*
** SLICE MANAGEMENT
*/

/*
** Slices are 1024-byte fragments from pages.  We maintain a free list of
** slices for those parts of the OS which don't need full 4096-byte chunks
** of space.
*/

/**
** Name:	carve_slices
**
** Split an allocated page into four slices and add
** them to the "free slices" list.
**
** @param page  Pointer to the page to be carved up
*/
static void carve_slices( void *page ) {

	// sanity check
	assert1( page != NULL );

#if TRACING_KMEM_FREELIST
	cio_printf( "KM: carve_slices(%08x)\n", (uint32_t) page );
#endif

	// create the four slices from it
	uint8_t *ptr = (uint8_t *) page;
	for( int i = 0; i < 4; ++i ) {
		km_slice_free( (void *) ptr );
		ptr += SZ_SLICE;
		++n_slices;
	}
}

/**
** Name:	km_slice_alloc
**
** Dynamically allocates a slice (1/4 of a page).  If no
** memory is available, we return NULL (unless ALLOC_FAIL_PANIC
** was defined, in which case we panic).
**
** @return a pointer to the allocated slice
*/
void *km_slice_alloc( void ) {

	// verify that km_init() was called first
	assert( km_initialized );

#if TRACING_KMEM_FREELIST
	cio_printf( "KM: sl_alloc()\n" );
#endif

	// if we are out of slices, create a few more
	if( free_slices.next == NULL ) {
		void *new = km_page_alloc();
		if( new == NULL ) {
			// can't get any more space
#if ALLOC_FAIL_PANIC
			PANIC( 0, "slice new alloc failed" );
#else
			return NULL;
#endif
		}
		carve_slices( new );
	}

	// take the first one from the free list
	void *slice = list_remove( &free_slices );
	assert( slice != NULL );
	--n_slices;

	// make it nice and shiny for the caller
	memclr( (void *) slice, SZ_SLICE );

	return( slice );
}

/**
** Name:	km_slice_free
**
** Returns a slice to the list of available slices.
**
** We make no attempt to merge slices, as we treat them as
** independent blocks of memory (like pages).
**
** @param[in] block  Pointer to the slice (1/4 page) to be freed
*/
void km_slice_free( void *block ) {

	// verify that km_init() was called first
	assert( km_initialized );

#if TRACING_KMEM_FREELIST
	cio_printf( "KM: sl_free(%08x)\n", (uint32_t) block );
#endif

	// just add it to the front of the free list
	list_add( &free_slices, block );
	--n_slices;
}
