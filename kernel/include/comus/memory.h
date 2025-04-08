/**
 * @file memory.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * Kernel memory functions
 */

#ifndef _MEMORY_H
#define _MEMORY_H

#include <stdint.h>
#include <stddef.h>

#define MMAP_MAX_ENTRY 64
#define PAGE_SIZE 4096

struct memory_segment {
	uint64_t addr;
	uint64_t len;
};

struct memory_map {
	uint32_t entry_count;
	struct memory_segment entries[MMAP_MAX_ENTRY];
};

/**
 * Initalize system memory allocator
 */
void memory_init(struct memory_map *map);

/**
 * @returns how much memory the system has
 */
uint64_t memory_total(void);

/**
 * @returns how much memory is free
 */
uint64_t memory_free(void);

/**
 * @returns how much memory is used
 */
uint64_t memory_used(void);

/**
 * Allocates at least len bytes of memory starting at
 * physical address addr. Returned address can be
 * any virtural address.
 *
 * @param addr - the physical address to map
 * @param len - the minimum length to map
 * @param writable - if this memory should be writable
 * @param user - if this memory should be user writable
 */
void *kmapaddr(void *addr, size_t len);

/**
 * Unmaps mapped address from the mmap function
 * @param addr - the address returned from mmap
 * @param len - the length allocated
 */
void kunmapaddr(void *addr);

/**
 * Allocates size_t bytes in memory
 *
 * @param size - the amount of bytes to allocate
 * @returns the address allocated or NULL on failure
 */
void *kalloc(size_t size);

/**
 * Rellocates a given allocated ptr to a new size of bytes in memory.
 * If ptr is NULL it will allocate new memory.
 *
 * @param ptr - the pointer to reallocate
 * @param size - the amount of bytes to reallocate to
 * @returns the address allocated or NULL on failure
 */
void *krealloc(void *ptr, size_t size);

/**
 * Frees an allocated pointer in memory
 *
 * @param ptr - the pointer to free
 */
void kfree(void *ptr);

/**
 * Attemps to load a mapped but not yet allocated page.
 *
 * @param virt_addr - the virtural address from either page allocation function
 *
 * @returns 0 on success and a negative error code on failure.
 */
int kload_page(void *virt_addr);

/**
 * Allocate a single page of memory
 *
 * @returns the address allocated or NULL on failure
 */
void *kalloc_page(void);

/**
 * Allocate size_t amount of contiguous virtual pages
 *
 * @param count - the number of pages to allocate
 * @returns the address allocated or NULL on failure
 */
void *kalloc_pages(size_t count);

/**
 * Free allocated pages.
 *
 * @param ptr - the pointer provided by alloc_page or alloc_pages
 */
void kfree_pages(void *ptr);

#endif /* memory.h */
