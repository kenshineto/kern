/**
 * @file virtalloc.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * Virtural address allocator functions
 */

#ifndef VIRTALLOC_H_
#define VIRTALLOC_H_

/**
 * Initalizes the virtual address allocator
 */
void virtaddr_init(void);

/**
 * Allocate a virtual address of length x pages
 * @param pages - x pages
 * @returns virt addr
 */
void *virtaddr_alloc(int pages);

/**
 * Free the virtual address from virtaddr_alloc
 * @param virtaddr - the addr to free
 * @returns number of pages used for virtaddr
 */
long virtaddr_free(void *virtaddr);

#endif /* virtalloc.h */
