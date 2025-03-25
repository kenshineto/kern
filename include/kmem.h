/**
** @file	kmem.h
**
** @author	Warren R. Carithers
** @author	Kenneth Reek
** @author	4003-506 class of 20013
**
** @brief	Support for dynamic memory allocation within the OS.
**
** This is a basic page allocator. Each allocation request returns
** a pointer to a single 4096-byte page of memory.
**
** The module also supports subddivision of pages into "slices",
** each of which is 1KB (i.e., 1/4 of a page).
*/

#ifndef KMEM_H_
#define KMEM_H_

#define KERNEL_SRC

// standard types etc.
#include <common.h>

/*
** General (C and/or assembly) definitions
*/

// Slab and slice sizes, in bytes

#define SZ_SLAB     SZ_PAGE
#define SZ_SLICE    (SZ_SLAB >> 2)

// memory limits
//
// these determine the range of memory addresses the kmem
// module will manage
//
// we won't map any memory below 1MB or above 1GB
#define KM_LOW_CUTOFF      NUM_1MB
#define KM_HIGH_CUTOFF     NUM_1GB

#ifndef ASM_SRC

/*
** Start of C-only definitions
*/

/*
** Types
*/

/*
** Globals
*/

/*
** Prototypes
*/

/**
** Name: km_init
**
** Find what memory is present on the system and
** construct the list of free memory blocks.
**
** Dependencies:
**    Must be called before any other init routine that uses
**    dynamic storage is called.
*/
void km_init( void );

/**
** Name:    km_dump
**
** Dump information about the free lists to the console. By default,
** prints only the list sizes; if 'addrs' is true, also dumps the list
** of page addresses; if 'all' is also true, dumps page addresses and
** slice addresses.
**
** @param addrs  Also dump page addresses
** @param both   Also dump slice addresses
*/
void km_dump( bool_t addrs, bool_t both );

/*
** Functions that manipulate free memory blocks.
*/

/**
** Name:    km_page_alloc
**
** Allocate a page of memory from the free list.
**
** @return a pointer to the beginning of the allocated page,
**         or NULL if no memory is available
*/
void *km_page_alloc( void );

/**
** Name:    km_page_free
**
** Returns a memory block to the list of available blocks,
** combining it with adjacent blocks if they're present.
**
** CRITICAL ASSUMPTION:  multi-page blocks will be freed one page
** at a time!
**
** @param[in] block   Pointer to the page to be returned to the free list
*/
void km_page_free( void *block );

/**
** Name:    km_slice_alloc
**
** Dynamically allocates a slice (1/4 of a page).  If no
** memory is available, we return NULL (unless ALLOC_FAIL_PANIC
** was defined, in which case we panic).
**
** @return a pointer to the allocated slice
*/
void *km_slice_alloc( void );

/**
** Name:    km_slice_free
**
** Returns a slice to the list of available slices.
**
** We make no attempt to merge slices, as they are independent
** blocks of memory (unlike pages).
**
** @param[in] block  Pointer to the slice (1/4 page) to be freed
*/
void km_slice_free( void *block );

#endif /* !ASM_SRC */

#endif
