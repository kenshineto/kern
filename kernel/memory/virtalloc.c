#include "lib/kio.h"
#include <lib.h>
#include <comus/memory.h>
#include <stdint.h>

#include "virtalloc.h"

static struct virt_addr_node *get_node_idx(struct virt_ctx *ctx, int idx)
{
	if (idx < BOOTSTRAP_VIRT_ALLOC_NODES) {
		return &ctx->bootstrap_nodes[idx];
	} else {
		return &ctx->alloc_nodes[idx - BOOTSTRAP_VIRT_ALLOC_NODES];
	}
}

static void update_node_ptrs(struct virt_addr_node *old,
							 struct virt_addr_node *new, int old_len,
							 int new_len)
{
	if (old == NULL)
		return;
	int idx = 0;
	for (int i = 0; i < old_len; i++) {
		struct virt_addr_node *o = &old[i];
		if (o && !o->is_used)
			continue;
		struct virt_addr_node *n = &new[idx++];
		*n = *o;
		if (n->prev != NULL)
			n->prev->next = n;
		if (n->next != NULL)
			n->next->prev = n;
	}
	for (int i = idx; i < new_len; i++) {
		struct virt_addr_node *n = &new[idx++];
		n->is_used = false;
	}
}

static struct virt_addr_node *get_node(struct virt_ctx *ctx)
{
	size_t count = BOOTSTRAP_VIRT_ALLOC_NODES + ctx->alloc_node_count;

	if (!ctx->is_allocating && ctx->used_node_count + 16 >= count) {
		ctx->is_allocating = true;
		int new_alloc = ctx->alloc_node_count * 2;
		if (new_alloc < 8)
			new_alloc = 8;
		struct virt_addr_node *new_nodes;
		new_nodes = kalloc(sizeof(struct virt_addr_node) * new_alloc);
		if (new_nodes == NULL)
			panic("virt addr alloc nodes is null");
		update_node_ptrs(ctx->alloc_nodes, new_nodes, ctx->alloc_node_count,
						 new_alloc);
		kfree(ctx->alloc_nodes);
		ctx->alloc_nodes = new_nodes;
		ctx->alloc_node_count = new_alloc;
		ctx->is_allocating = false;
		count = BOOTSTRAP_VIRT_ALLOC_NODES + ctx->alloc_node_count;
	}

	size_t idx = ctx->free_node_start;
	for (; idx < count; idx++) {
		struct virt_addr_node *node = get_node_idx(ctx, idx);
		if (!node->is_used) {
			ctx->used_node_count++;
			return node;
		}
	}

	panic("could not get virtaddr node");
}

static void free_node(struct virt_ctx *ctx, struct virt_addr_node *node)
{
	node->is_used = false;
	ctx->used_node_count--;
}

void virtaddr_init(struct virt_ctx *ctx)
{
	struct virt_addr_node init = {
		.start = 0x50000000,
		.end = 0x1000000000000, // 48bit memory address max
		.next = NULL,
		.prev = NULL,
		.is_alloc = false,
		.is_used = true,
	};
	memset(ctx, 0, sizeof(struct virt_ctx));
	ctx->bootstrap_nodes[0] = init;
	ctx->alloc_nodes = NULL;
	ctx->start_node = &ctx->bootstrap_nodes[0];
	ctx->free_node_start = 0;
	ctx->alloc_node_count = 0;
	ctx->used_node_count = 0;
	ctx->is_allocating = false;
}

int virtaddr_clone(struct virt_ctx *old, struct virt_ctx *new)
{
	// copy over data
	memcpy(new, old, sizeof(struct virt_ctx));

	// allocate new space
	new->alloc_nodes =
		kalloc(sizeof(struct virt_addr_node) * new->alloc_node_count);
	if (new->alloc_nodes == NULL)
		return 1;

	// update prev/next in new allocation space
	update_node_ptrs(old->alloc_nodes, new->alloc_nodes, old->alloc_node_count,
					 new->alloc_node_count);

	// update bootstrap nodes
	for (size_t i = 0; i < new->used_node_count; i++) {
		struct virt_addr_node *prev, *next;

		if (i >= BOOTSTRAP_VIRT_ALLOC_NODES)
			break;

		// get prev
		prev = i > 0 ? &new->bootstrap_nodes[i - 1] : NULL;
		next = i < BOOTSTRAP_VIRT_ALLOC_NODES - 1 ?
				   &new->bootstrap_nodes[i + 1] :
				   NULL;

		new->bootstrap_nodes[i].prev = prev;
		new->bootstrap_nodes[i].next = next;
	}

	// get starting node
	new->start_node = &new->bootstrap_nodes[0]; // for now

	return 0;
}

static void merge_back(struct virt_ctx *ctx, struct virt_addr_node *node)
{
	while (node->prev) {
		if (node->is_alloc != node->prev->is_alloc)
			break;
		struct virt_addr_node *temp = node->prev;
		node->start = temp->start;
		node->prev = temp->prev;
		if (temp->prev)
			temp->prev->next = node;
		free_node(ctx, temp);
	}
	if (node->prev == NULL) {
		ctx->start_node = node;
	}
}

static void merge_forward(struct virt_ctx *ctx, struct virt_addr_node *node)
{
	while (node->next) {
		if (node->is_alloc != node->next->is_alloc)
			break;
		struct virt_addr_node *temp = node->next;
		node->end = temp->end;
		node->next = temp->next;
		if (temp->next)
			temp->next->prev = node;
		free_node(ctx, temp);
	}
}

void *virtaddr_alloc(struct virt_ctx *ctx, int n_pages)
{
	if (n_pages < 1)
		return NULL;
	long n_length = n_pages * PAGE_SIZE;
	struct virt_addr_node *node = ctx->start_node;

	for (; node != NULL; node = node->next) {
		long length = node->end - node->start;
		if (node->is_alloc)
			continue;

		if (length < n_length)
			continue;

		return (void*)node->start;
	}

	return NULL;
}

int virtaddr_take(struct virt_ctx *ctx, const void *virt, int n_pages)
{
	if (n_pages < 1)
		return 0;

	long n_length = n_pages * PAGE_SIZE;
	struct virt_addr_node *node = ctx->start_node;

	for (; node != NULL; node = node->next) {
		if (node->is_alloc)
			continue;

		if (node->start > (uintptr_t)virt ||
			node->end < (uintptr_t)virt + n_length)
			continue;

		// create new node on left
		if (node->start < (uintptr_t) virt) {
			struct virt_addr_node *left = get_node(ctx);
			left->next = node;
			left->prev = node->prev;
			left->start = node->start;
			left->end = (uintptr_t) virt;
			left->is_used = true;
			left->is_alloc = false;
			node->prev->next = left;
			node->prev = left;
		}

		// create new node on right
		if (node->end > (uintptr_t) virt + n_length) {
			struct virt_addr_node *right = get_node(ctx);
			right->prev = node;
			right->next = node->next;
			right->start = (uintptr_t) virt + n_length;
			right->end = node->end;
			right->is_used = true;
			right->is_alloc = false;
			node->next->prev = right;
			node->next = right;
		}

		node->start = (uintptr_t) virt;
		node->end = node->start + n_length;
		node->is_alloc = true;
		node->is_used = true;

		return 0;
	}

	return 1;
}

long virtaddr_free(struct virt_ctx *ctx, const void *virtaddr)
{
	if (virtaddr == NULL)
		return -1;

	uintptr_t virt = (uintptr_t)virtaddr;

	if (virt % PAGE_SIZE)
		return -1; // not page aligned, we did not give this out!!!

	struct virt_addr_node *node = ctx->start_node;

	for (; node != NULL; node = node->next) {
		if (node->start == virt) {
			int length = node->end - node->start;
			int pages = length / PAGE_SIZE;
			merge_back(ctx, node);
			merge_forward(ctx, node);
			return pages;
		}
	}

	return -1;
}

void virtaddr_cleanup(struct virt_ctx *ctx)
{
	kfree(ctx->alloc_nodes);
}
