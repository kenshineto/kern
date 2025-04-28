/**
 * @file virtalloc.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * Virtural address allocator functions
 */

#ifndef VIRTALLOC_H_
#define VIRTALLOC_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define BOOTSTRAP_VIRT_ALLOC_NODES 64

struct virt_addr_node {
	/// first virtural address
	uintptr_t start;
	/// last virtural address
	uintptr_t end;
	/// next node in linked list
	struct virt_addr_node *next;
	/// prev node in linked list
	struct virt_addr_node *prev;
	/// if this node is storing any allocated data
	uint8_t is_alloc;
	/// if this node is in use by virtalloc
	uint8_t is_used;
};

struct virt_ctx {
	/// bootstrap nodes for the context (not in heap)
	struct virt_addr_node bootstrap_nodes[BOOTSTRAP_VIRT_ALLOC_NODES];
	/// heap allocated nodes
	struct virt_addr_node *alloc_nodes;
	/// start node
	struct virt_addr_node *start_node;
	/// index of first free node
	size_t free_node_start;
	/// number of heap allocated nodes
	size_t alloc_node_count;
	/// number of used nodes
	size_t used_node_count;
	/// if we are currently allocating (recursion check)
	bool is_allocating;
};

/**
 * Initalizes the virtual address allocator
 */
void virtaddr_init(struct virt_ctx *ctx);

/**
 * Clone the virtual address allocator
 */
int virtaddr_clone(struct virt_ctx *old, struct virt_ctx *new);

/**
 * Allocate a virtual address of length x pages
 * @param pages - x pages
 * @returns virt addr
 */
void *virtaddr_alloc(struct virt_ctx *ctx, int pages);

/**
 * Take (yoink) a predefined virtual address of length x pages
 * @param virt - the start of the vitural address to take
 * @param pages - x pages
 * @returns 0 on success, 1 on err
 */
int virtaddr_take(struct virt_ctx *ctx, const void *virt, int pages);

/**
 * Free the virtual address from virtaddr_alloc
 * @param virtaddr - the addr to free
 * @returns number of pages used for virtaddr
 */
long virtaddr_free(struct virt_ctx *ctx, const void *virtaddr);

/**
 * Cleans up heap allocations and frees the virtalloc context
 */
void virtaddr_cleanup(struct virt_ctx *ctx);

#endif /* virtalloc.h */
