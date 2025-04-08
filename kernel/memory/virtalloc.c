#include <lib.h>
#include <comus/memory.h>

#include "virtalloc.h"

struct addr_node {
	uintptr_t start;
	uintptr_t end;
	struct addr_node *next;
	struct addr_node *prev;
	uint8_t is_alloc; // if node is storing allocated data
	uint8_t is_used; // if node is in use by virtalloc
};

#define BSS_NODES 64
static struct addr_node bootstrap_nodes[BSS_NODES];
static struct addr_node *alloc_nodes = NULL;
static size_t free_node_start = 0;
static size_t alloc_node_count = 0;
static size_t used_node_count = 0;
static bool is_allocating = false;

static struct addr_node *start_node = NULL;

static struct addr_node *get_node_idx(int idx)
{
	if (idx < BSS_NODES) {
		return &bootstrap_nodes[idx];
	} else {
		return &alloc_nodes[idx - BSS_NODES];
	}
}

static void update_node_ptrs(struct addr_node *old, struct addr_node *new,
							 int old_len, int new_len)
{
	if (old == NULL)
		return;
	int idx = 0;
	for (int i = 0; i < old_len; i++) {
		struct addr_node *o = &old[i];
		if (o && !o->is_used)
			continue;
		struct addr_node *n = &new[idx++];
		*n = *o;
		if (n->prev != NULL)
			n->prev->next = n;
		if (n->next != NULL)
			n->next->prev = n;
	}
	for (int i = idx; i < new_len; i++) {
		struct addr_node *n = &new[idx++];
		n->is_used = false;
	}
}

static struct addr_node *get_node(void)
{
	size_t count = BSS_NODES + alloc_node_count;

	if (!is_allocating && used_node_count + 16 >= count) {
		is_allocating = true;
		int new_alloc = alloc_node_count * 2;
		if (new_alloc < 8)
			new_alloc = 8;
		struct addr_node *new_nodes;
		new_nodes = kalloc(sizeof(struct addr_node) * new_alloc);
		if (new_nodes == NULL)
			panic("virt addr alloc nodes is null");
		update_node_ptrs(alloc_nodes, new_nodes, alloc_node_count, new_alloc);
		kfree(alloc_nodes);
		alloc_nodes = new_nodes;
		alloc_node_count = new_alloc;
		is_allocating = false;
		count = BSS_NODES + alloc_node_count;
	}

	size_t idx = free_node_start;
	for (; idx < count; idx++) {
		struct addr_node *node = get_node_idx(idx);
		if (!node->is_used) {
			used_node_count++;
			return node;
		}
	}

	panic("could not get virtaddr node");
}

static void free_node(struct addr_node *node)
{
	node->is_used = false;
	used_node_count--;
}

void virtaddr_init(void)
{
	struct addr_node init = {
		.start = 0x400000, // third page table
		.end = 0x1000000000000, // 48bit memory address max
		.next = NULL,
		.prev = NULL,
		.is_alloc = false,
		.is_used = true,
	};
	memsetv(bootstrap_nodes, 0, sizeof(bootstrap_nodes));
	bootstrap_nodes[0] = init;
	start_node = &bootstrap_nodes[0];
}

static void merge_back(struct addr_node *node)
{
	while (node->prev) {
		if (node->is_alloc != node->prev->is_alloc)
			break;
		struct addr_node *temp = node->prev;
		node->start = temp->start;
		node->prev = temp->prev;
		if (temp->prev)
			temp->prev->next = node;
		free_node(temp);
	}
	if (node->prev == NULL) {
		start_node = node;
	}
}

static void merge_forward(struct addr_node *node)
{
	while (node->next) {
		if (node->is_alloc != node->next->is_alloc)
			break;
		struct addr_node *temp = node->next;
		node->end = temp->end;
		node->next = temp->next;
		if (temp->next)
			temp->next->prev = node;
		free_node(temp);
	}
}

void *virtaddr_alloc(int n_pages)
{
	if (n_pages < 1)
		return NULL;
	long n_length = n_pages * PAGE_SIZE;
	struct addr_node *node = start_node;

	for (; node != NULL; node = node->next) {
		long length = node->end - node->start;
		if (node->is_alloc)
			continue;

		if (length >= n_length) {
			struct addr_node *new = get_node();
			if (node->prev != NULL) {
				node->prev->next = new;
			} else {
				start_node = new;
			}
			new->next = node;
			new->prev = node->prev;
			node->prev = new;
			new->start = node->start;
			new->end = new->start + n_length;
			node->start = new->end;
			new->is_alloc = true;
			new->is_used = true;
			new->next = node;
			void *mem = (void *)new->start;
			merge_back(new);
			merge_forward(new);
			return mem;
		}
	}

	return NULL;
}

long virtaddr_free(void *virtaddr)
{
	uintptr_t virt = (uintptr_t)virtaddr;

	if (virt % PAGE_SIZE)
		return -1; // not page aligned, we did not give this out!!!

	struct addr_node *node = start_node;

	for (; node != NULL; node = node->next) {
		if (node->start == virt) {
			int length = node->end - node->start;
			int pages = length / PAGE_SIZE;
			merge_back(node);
			merge_forward(node);
			return pages;
		}
	}

	return -1;
}
