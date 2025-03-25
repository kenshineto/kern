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
** and contains a series of blocks which are multiples of 4K bytes and
** which are aligned at 4K boundaries; they are held in the free list
** in order by base address.
**
** The "page" allocator allows allocation of one or more 4K blocks
** at a time.  Requests are made for a specific number of 4K pages;
** the allocator locates the first free list entry that contains at
** least the requested amount of space.  If that entry is the exact
** size requested, it is unlinked and returned; otherwise, the entry
** is split into a chunk of the requested size and the remainder.
** The chunk is returned, and the remainder replaces the original
** block in the free list.  On deallocation, the block is inserted
** at the appropriate place in the free list, and physically adjacent
** blocks are coalesced into single, larger blocks.  If a multi-page
** block is allocated, it should be deallocated one page at a time,
** because there is no record of the size of the original allocation -
** all we know is that it is N*4K bytes in length, so it's up to the
** requesting code to figure this out.
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
*/

#define KERNEL_SRC

#include <common.h>

// all other framework includes are next
#include <lib.h>

#include <x86/arch.h>
#include <x86/bios.h>
#include <bootstrap.h>
#include <cio.h>

#include <kmem.h>

/*
** PRIVATE DEFINITIONS
*/

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
** This structure keeps track of a single block of memory.  All blocks
** are multiples of the base size (currently, 4KB).
*/

typedef struct block_s {
	uint32_t pages;			// length of this block, in pages
	struct   block_s *next;	// pointer to the next free block
} block_t;

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
} __attribute__((packed)) region_t;

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
static block_t *free_pages;
static block_t *free_slices;

// initialization status
static int km_initialized = 0;

/*
** IMPORTED GLOBAL VARIABLES
*/

extern int _end;	// end of the BSS section - provided by the linker

/*
** FUNCTIONS
*/

/*
** FREE LIST MANAGEMENT
*/

/**
** Name:	add_block
**
** Add a block to the free list. The contents of the block
** will be modified.
**
** @param[in] base   Base address of the block
** @param[in] length Block length, in bytes
*/
static void add_block( uint32_t base, uint32_t length ) {
	block_t *block;

	// don't add it if it isn't at least 4K
	if( length < SZ_PAGE ) {
		return;
	}

	// only want to add multiples of 4K; check the lower bits
	if( (length & 0xfff) != 0 ) {
		// round it down to 4K
		length &= 0xfffff000;
	}

	// create the "block"

	block = (block_t *) base;
	block->pages = B2P(length);
	block->next = NULL;

#if TRACING_KMEM_FREE
	cio_printf( "KM: add(%08x,%u): addr %08x len %u\n",
				base, length, (uint32_t)block, block->length );
#endif

	/*
	** We maintain the free list in order by address, to simplify
	** coalescing adjacent free blocks.
	**
	** Handle the easiest case first.
	*/

	if( free_pages == NULL ) {
		free_pages = block;
		return;
	}

	/*
	** Unfortunately, it's not always that easy....
	**
	** Find the correct insertion spot.
	*/

	block_t *prev, *curr;

	prev = NULL;
	curr = free_pages;

	while( curr && curr < block ) {
		prev = curr;
		curr = curr->next;
	}

	// the new block always points to its successor
	block->next = curr;

	/*
	** If prev is NULL, we're adding at the front; otherwise,
	** we're adding after some other entry (middle or end).
	*/

	if( prev == NULL ) {
		// sanity check - both pointers can't be NULL
		assert( curr );
		// add at the beginning
		free_pages = block;
	} else {
		// inserting in the middle or at the end
		prev->next = block;
	}
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
	uint64_t cutoff;

#if TRACING_INIT
	// announce that we're starting initialization
	cio_puts( " Kmem" );
#endif

	// initially, nothing in the free lists
	free_slices = NULL;
	free_pages = NULL;

	/*
	** We ignore all memory below the end of our OS.  In theory,
	** we should be able to re-use much of that space; in practice,
	** this is safer.
	*/

	// set our cutoff point as the end of the BSS section
	cutoff = (uint32_t) &_end;

	// round it up to the next multiple of 4K (0x1000)
	if( cutoff & 0xfffLL ) {
		cutoff &= 0xfffff000LL;
		cutoff += 0x1000LL;
	}

	// get the list length
	entries = *((int32_t *) MMAP_ADDRESS);

#if TRACING_KEMEM
	cio_printf( "\nKmem: %d regions\n", entries );
#endif

	// if there are no entries, we have nothing to do!
	if( entries < 1 ) {  // note: entries == -1 could occur!
		return;
	}

	// iterate through the entries, adding things to the freelist

	region = ((region_t *) (MMAP_ADDRESS + 4));

	for( int i = 0; i < entries; ++i, ++region ) {

#if TRACING_KMEM
		// report this region
		cio_printf( "%3d: ", i );
		cio_printf( " base %08x%08x",
				region->base.HIGH, region->base.LOW );
		cio_printf( " len %08x%08x",
				region->length.HIGH, region->length.LOW );
		cio_printf( " type %08x acpi %08x",
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
		**  Region is above the 4GB address limit
		**
		** Currently, only "normal" (type 1) regions are considered
		** "usable" for our purposes.  We could potentially expand
		** this to include ACPI "reclaimable" memory.
		*/

		// first, check the ACPI one-bit flags

		if( ((region->acpi) & REGION_IGNORE) == 0 ) {
			cio_puts( " IGN\n" );
			continue;
		}

		if( ((region->acpi) & REGION_NONVOL) != 0 ) {
			cio_puts( " NVOL\n" );
			continue;  // we'll ignore this, too
		}

		// next, the region type

		if( (region->type) != REGION_USABLE ) {
			cio_puts( " RCLM\n" );
			continue;  // we won't attempt to reclaim ACPI memory (yet)
		}

		// OK, we have a "normal" memory region - verify that it's usable

		// ignore it if it's above 4GB
		if( region->base.HIGH != 0 ) {
			cio_puts( " 4GB+\n" );
			continue;
		}

		// grab the two 64-bit values to simplify things
		uint64_t base   = region->base.all;
		uint64_t length = region->length.all;

		// see if it's below our arbitrary cutoff point
		if( base < cutoff ) {

			// is the whole thing too low, or just part?
			if( (base + length) < cutoff ) {
				// it's all below the cutoff!
				cio_puts( " LOW\n" );
				continue;
			}

			// recalculate the length, starting at our cutoff point
			uint64_t loss = cutoff - base;

			// reset the length and the base address
			length -= loss;
			base = cutoff;
		}

		// see if it extends beyond the 4GB boundary

		if( (base + length) > ADDR_32_MAX ) {

			// OK, it extends beyond the 32-bit limit; figure out
			// how far over it goes, and lop off that portion

			uint64_t loss = (base + length) - ADDR_64_FIRST;
			length -= loss;
		}

		// we survived the gauntlet - add the new block

		cio_puts( " OK\n" );

		uint32_t b32 = base   & ADDR_LOW_HALF;
		uint32_t l32 = length & ADDR_LOW_HALF;

		add_block( b32, l32 );
	}

	// record the initialization
	km_initialized = 1;
}

/**
** Name:	km_dump
**
** Dump the current contents of the free list to the console
*/
void km_dump( void ) {
	block_t *block;

	cio_printf( "&_free_pages=%08x\n", &_free_pages );

	for( block = _free_pages; block != NULL; block = block->next ) {
		cio_printf(
			"block @ 0x%08x 0x%08x pages (ends at 0x%08x) next @ 0x%08x\n",
				block, block->pages, P2B(block->pages) + (uint32_t) block,
				block->next );
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
** @param[in] count  Number of contiguous pages desired
**
** @return a pointer to the beginning of the first allocated page,
**		   or NULL if no memory is available
*/
void *km_page_alloc( unsigned int count ) {

	assert( km_initialized );

	// make sure we actually need to do something!
	if( count < 1 ) {
		return( NULL );
	}

#if TRACING_KMEM_FREE
	cio_printf( "KM: pg_alloc(%u)", count );
#endif

	/*
	** Look for the first entry that is large enough.
	*/

	// pointer to the current block
	block_t *block = free_pages;

	// pointer to where the pointer to the current block is
	block_t **pointer = &free_pages;

	while( block != NULL && block->pages < count ){
		pointer = &block->next;
		block = *pointer;
	}

	// did we find a big enough block?
	if( block == NULL ){
		// nope!
#if TRACING_KMEM_FREE
		cio_puts( " FAIL\n" );
#endif
		return( NULL );
	}

	// found one!  check the length

	if( block->pages == count ) {

		// exactly the right size - unlink it from the list

		*pointer = block->next;

	} else {

		// bigger than we need - carve the amount we need off
		// the beginning of this block

		// remember where this chunk begins
		block_t *chunk = block;

		// how much space will be left over?
		int excess = block->pages - count;

		// find the start of the new fragment
		block_t *fragment = (block_t *) ( (uint8_t *) block + P2B(count) );

		// set the length and link for the new fragment
		fragment->pages = excess;
		fragment->next  = block->next;

		// replace this chunk with the fragment
		*pointer = fragment;

		// return this chunk
		block = chunk;
	}

#if TRACING_KMEM_FREE
	cio_printf( " -> %08x\n", (uint32_t) block );
#endif

	return( block );
}

/**
** Name:	km_page_free
**
** Returns a memory block to the list of available blocks,
** combining it with adjacent blocks if they're present.
**
** CRITICAL ASSUMPTION:  multi-page blocks will be freed one page
** at a time!
**
** @param[in] block   Pointer to the page to be returned to the free list
*/
void km_page_free( void *block ){
	block_t *used;
	block_t *prev;
	block_t *curr;

	assert( km_initialized );

	/*
	** Don't do anything if the address is NULL.
	*/
	if( block == NULL ){
		return;
	}

#if TRACING_KMEM_FREE
	cio_printf( "KM: pg_free(%08x)", (uint32_t) block );
#endif

	used = (block_t *) block;

	/*
	** CRITICAL ASSUMPTION
	**
	** We assume that any multi-page block that is being freed will
	** be freed one page at a time.  We make this assumption because we
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

	used->pages = 1;

	/*
	** Advance through the list until current and previous
	** straddle the place where the new block should be inserted.
	*/
	prev = NULL;
	curr = free_pages;

	while( curr != NULL && curr < used ){
		prev = curr;
		curr = curr->next;
	}

	/*
	** At this point, we have the following list structure:
	**
	**   ....	BLOCK		BLOCK		....
	**			(*prev)  ^  (*curr)
	**					 |
	**				"used" goes here
	**
	** We may need to merge the inserted block with either its
	** predecessor or its successor (or both).
	*/

	/*
	** If this is not the first block in the resulting list,
	** we may need to merge it with its predecessor.
	*/
	if( prev != NULL ){

		// There is a predecessor.  Check to see if we need to merge.
		if( adjacent( prev, used ) ){

			// yes - merge them
			prev->pages += used->pages;

			// the predecessor becomes the "newly inserted" block,
			// because we still need to check to see if we should
			// merge with the successor
			used = prev;

		} else {

			// Not adjacent - just insert the new block
			// between the predecessor and the successor.
			used->next = prev->next;
			prev->next = used;

		}

	} else {

		// Yes, it is first.  Update the list pointer to insert it.
		used->next = free_pages;
		free_pages = used;

	}

	/*
	** If this is not the last block in the resulting list,
	** we may (also) need to merge it with its successor.
	*/
	if( curr != NULL ){

		// No.  Check to see if it should be merged with the successor.
		if( adjacent( used, curr ) ){

			// Yes, combine them.
			used->next = curr->next;
			used->pages += curr->pages;

		}
	}
}

/*
** SLICE MANAGEMENT
*/

/*
** Slices are 1024-byte fragments from pages.  We maintain a free list of
** slices for those parts of the OS which don't need full 4096-byte chunks
** of space (e.g., the QNode and Queue allocators).
*/

/**
** Name:	carve_slices
**
** Allocate a page and split it into four slices;  If no
** memory is available, we panic.
*/
static void carve_slices( void ) {
	void *page;

	// get a page
	page = km_page_alloc( 1 );

	// allocation failure is a show-stopping problem
	assert( page );

	// we have the page; create the four slices from it
	uint8_t *ptr = (uint8_t *) page;
	for( int i = 0; i < 4; ++i ) {
		km_slice_free( (void *) ptr );
		ptr += SZ_SLICE;
	}
}

/**
** Name:	km_slice_alloc
**
** Dynamically allocates a slice (1/4 of a page).  If no
** memory is available, we panic.
**
** @return a pointer to the allocated slice
*/
void *km_slice_alloc( void ) {
	block_t *slice;

	assert( km_initialized );

#if TRACING_KMEM_FREE
	cio_printf( "KM: sl_alloc()\n" );
#endif

	// if we are out of slices, create a few more
	if( free_slices == NULL ) {
		carve_slices();
	}

	// take the first one from the free list
	slice = free_slices;
	assert( slice != NULL );

	// unlink it
	free_slices = slice->next;

	// make it nice and shiny for the caller
	memclr( (void *) slice, SZ_SLICE );

	return( slice );
}

/**
** Name:	km_slice_free
**
** Returns a slice to the list of available slices.
**
** We make no attempt to merge slices, as they are independent
** blocks of memory (unlike pages).
**
** @param[in] block  Pointer to the slice (1/4 page) to be freed
*/
void km_slice_free( void *block ) {
	block_t *slice = (block_t *) block;

	assert( km_initialized );

#if TRACING_KMEM_FREE
	cio_printf( "KM: sl_free(%08x)\n", (uint32_t) block );
#endif

	// just add it to the front of the free list
	slice->pages = SZ_SLICE;
	slice->next = free_slices;
	free_slices = slice;
}
