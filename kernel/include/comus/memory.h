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
void *mapaddr(void *addr, size_t len);

/**
 * Unmaps mapped address from the mmap function
 * @param addr - the address returned from mmap
 * @param len - the length allocated
 */
void unmapaddr(void *addr);

/**
 * Attemps to load a mapped but not yet allocated page.
 *
 * @param virt_addr - the virtural address from either page allocation function
 *
 * @returns 0 on success and a negative error code on failure.
 */
int load_page(void *virt_addr);

#endif /* memory.h */
