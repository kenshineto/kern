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

/**
 * Initalize the physical page allocator
 */
void physalloc_init(struct memory_map *map);

/**
 * Allocates a single physical page in memory
 * @preturns the physical address of the page
 */
void *alloc_phys_page(void);

/**
 * Allocates count physical pages in memory
 * @returns the physical address of the first page
 */
void *alloc_phys_pages(size_t count);

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

#endif /* physalloc.h */
