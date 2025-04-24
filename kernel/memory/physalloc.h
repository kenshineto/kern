/**
 * @file physalloc.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * Physical page allocator functions
 */

#ifndef PHYSALLOC_H_
#define PHYSALLOC_H_

#include <comus/memory.h>

/// Represents some contiguous physical pages
struct phys_page_slice {
	void *pagestart;
	size_t num_pages;
};

#define PHYS_PAGE_SLICE_NULL \
	((struct phys_page_slice){ .pagestart = NULL, .num_pages = 0 })

/**
 * Initalize the physical page allocator
 */
void physalloc_init(struct memory_map *map);

/*
 * Allocates the first page(s) it finds. Returns a pointer to that page
 * and, if there are (up to max_pages) extra pages free after it, it allocates
 * them as well.
 *
 * @param max_pages - the maximum number of pages to mark as allocated
 * @returns a slice of all of the allocated pages, num_pages will be
 * <= max_pages
 */
struct phys_page_slice alloc_phys_page_withextra(size_t max_pages);

/**
 * Allocates a single physical page in memory
 * @preturns the physical address of the page
 */
void *alloc_phys_page(void);

/**
 * Allocates count contiguous physical pages in memory
 * @returns the physical address of the first page, or NULL if no
 * contiguous pages exist.
 */
void *alloc_phys_pages_exact(size_t count);

/**
* Frees a single physical page in memory
 * @param ptr - the physical address of the page
 */
void free_phys_page(void *ptr);

/**
 * Frees count physical pages in memory
 * @param ptr - the physical address of the first page
 * @param count - the number of pages in the list
 */
void free_phys_pages(void *ptr, size_t count);

/**
 * Frees a slice of physical pages in memory
 * @param slice - the pages to free
 */
void free_phys_pages_slice(struct phys_page_slice slice);

#endif /* physalloc.h */
