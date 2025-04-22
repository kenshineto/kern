/**
 * @file memory.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * Kernel memory functions
 */

#ifndef _MEMORY_H
#define _MEMORY_H

#include <comus/limits.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PAGE_SIZE 4096

#define F_PRESENT 0x001
#define F_WRITEABLE 0x002
#define F_UNPRIVILEGED 0x004
#define F_WRITETHROUGH 0x008
#define F_CACHEDISABLE 0x010
#define F_ACCESSED 0x020
#define F_DIRTY 0x040
#define F_MEGABYTE 0x080
#define F_GLOBAL 0x100

#define SEG_TYPE_FREE 0
#define SEG_TYPE_RESERVED 1
#define SEG_TYPE_ACPI 2
#define SEG_TYPE_HIBERNATION 3
#define SEG_TYPE_DEFECTIVE 4
#define SEG_TYPE_EFI 5

struct memory_segment {
	uint64_t addr;
	uint64_t len;
	uint32_t type;
};

struct memory_map {
	uint32_t entry_count;
	struct memory_segment entries[N_MMAP_ENTRY];
};

typedef struct mem_ctx_s *mem_ctx_t;
extern mem_ctx_t kernel_mem_ctx;

/**
 * Initalize system memory allocator
 */
void memory_init(void);

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
 * Reports system memory usage and map
 */
void memory_report(void);

/**
 * Allocate a new memory context
 *
 * @returns pointer context or NULL on failure
 */
mem_ctx_t mem_ctx_alloc(void);

/**
 * Clone a current memory context into a new one
 *
 * @param ctx - the memory context
 * @param cow - mark all of the pages as copy on write
 *
 * @returns pointer context or NULL on failure
 */
mem_ctx_t mem_ctx_clone(mem_ctx_t ctx, bool cow);

/**
 * Free a memory context into a new one
 *
 * @param ctx - the memory context
 */
void mem_ctx_free(mem_ctx_t ctx);

/**
 * Switch into a different memory context
 *
 * @param ctx - the memory context
 */
void mem_ctx_switch(mem_ctx_t ctx);

/**
 * Allocates at least len bytes of memory starting at
 * physical address addr. Returned address can be
 * any virtural address.
 *
 * @param phys - the physical address to map
 * @param virt - the virtural address to map (or NULL for any virt addr)
 * @param len - the minimum length in bytes to map
 * @param flags - memory flags (F_PRESENT will always be set)
 * @returns the mapped vitural address to phys, or NULL on failure
 */
void *mem_mapaddr(mem_ctx_t ctx, void *phys, void *virt, size_t len,
				  unsigned int flags);

/**
 * Unmaps mapped address from the kmapaddr function
 * @param virt - the vitural address returned from kmapaddr
 */
void mem_unmapaddr(mem_ctx_t ctx, void *virt);

/**
 * Allocate a single page of memory with the given paging structure
 *
 * @param ctx - the memory context
 * @param lazy - if to lazy allocate pages (alloc on fault)
 * @returns the vitural address aloocated or NULL on failure
 */
void *mem_alloc_page(mem_ctx_t ctx, bool lazy);

/**
 * Allocate a single page of memory at the given vitural address with the given paging structure
 *
 * @param ctx - the memory context
 * @param virt - the vitural address to allocate at
 * @param lazy - if to lazy allocate pages (alloc on fault)
 * @returns the vitural address aloocated or NULL on failure
 */
void *mem_alloc_page_at(mem_ctx_t ctx, void *virt, bool lazy);

/**
 * Allocate size_t amount of contiguous virtual pages with the given paging structure
 *
 * @param ctx - the memory context
 * @param count - the number of pages to allocate
 * @param lazy - if to lazy allocate pages (alloc on fault)
 * @returns the address allocated or NULL on failure
 */
void *mem_alloc_pages(mem_ctx_t ctx, size_t count, bool lazy);

/**
 * Allocate size_t amount of contiguous virtual pages at a given virtural address with the given paging structure
 *
 * @param ctx - the memory context
 * @param count - the number of pages to allocate
 * @param virt - the vitural address to allocate at
 * @param lazy - if to lazy allocate pages (alloc on fault)
 * @returns the address allocated or NULL on failure
 */
void *mem_alloc_pages_at(mem_ctx_t ctx, size_t count, void *virt, bool lazy);

/**
 * Free allocated pages with the given paging structure.
 *
 * @param ptr - the pointer provided by alloc_page or alloc_pages
 */
void mem_free_pages(mem_ctx_t ctx, void *ptr);

/**
 * Load a not allocated but properly mapped page
 *
 * @returns 0 on success, negative error code on failure
 */
int mem_load_page(mem_ctx_t ctx, void *virt);

/**
 * Allocates at least len bytes of memory starting at
 * physical address addr. Returned address can be
 * any virtural address.
 *
 * @param phys - the physical address to map
 * @param virt - the virtural address to map (or NULL for any virt addr)
 * @param len - the minimum length in bytes to map
 * @param flags - memory flags (F_PRESENT will always be set)
 * @returns the mapped vitural address to phys, or NULL on failure
 */
void *kmapaddr(void *phys, void *virt, size_t len, unsigned int flags);

/**
 * Unmaps mapped address from the kmapaddr function
 * @param virt - the vitural address returned from kmapaddr
 */
void kunmapaddr(void *virt);

/**
 * Allocate a single page of memory
 *
 * @returns the vitural address allocated or NULL on failure
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

/**
 * Load a not allocated but properly mapped page
 *
 * @returns 0 on success, negative error code on failure
 */
int kload_page(void *virt);

#endif /* memory.h */
