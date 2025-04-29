#include <lib.h>
#include <comus/memory.h>

#define MAGIC 0xBEEFCAFE

struct page_header {
	struct page_header *next;
	struct page_header *prev;
	size_t
		node_number; // all headers on the same page alloc have the same node number (so they can be merged)
	size_t
		free; // free space after the node (if its the last node in the alloc block)
	size_t used; // how much space this allocation is using
	uint64_t magic;
};

static const size_t header_len = sizeof(struct page_header);
static struct page_header *start_header = NULL;
static struct page_header *end_header = NULL;

static struct page_header *get_header(void *ptr)
{
	struct page_header *header =
		(struct page_header *)((uintptr_t)ptr - header_len);

	// PERF: do we want to make sure this pointer is paged
	// before reading it???
	if (header->magic != MAGIC) {
		return NULL; // invalid pointer
	}

	return header;
}

static void *alloc_new(size_t size)
{
	size_t pages = ((size + header_len) / PAGE_SIZE) + 1;

	void *addr = kalloc_pages(pages);
	void *mem = (char *)addr + header_len;

	size_t total = pages * PAGE_SIZE;
	size_t free = total - (size + header_len);

	if (addr == NULL) {
		return NULL;
	}

	size_t node;
	if (end_header != NULL) {
		node = end_header->node_number + 1;
	} else {
		node = 0;
	}

	struct page_header *header = addr;
	memsetv(header, 0, sizeof(struct page_header));
	header->magic = MAGIC;
	header->used = size;
	header->free = free;
	header->prev = end_header;
	header->next = NULL;
	header->node_number = node;

	if (start_header == NULL) {
		start_header = header;
	}

	if (end_header != NULL) {
		end_header->next = header;
	} else {
		end_header = header;
	}

	return mem;
}

static void *alloc_block(size_t size, struct page_header *block)
{
	struct page_header *header =
		(struct page_header *)((char *)block + block->used + header_len);

	size_t free = block->free - (size + header_len);
	block->free = 0;

	header->magic = MAGIC;
	header->used = size;
	header->free = free;
	header->prev = block;
	header->next = block->next;
	block->next = header;
	header->node_number = block->node_number;

	void *mem = (char *)header + header_len;

	return mem;
}

void *kalloc(size_t size)
{
	struct page_header *header = start_header;

	for (; header != NULL; header = header->next) {
		size_t free = header->free;
		if (free < header_len)
			continue;
		if (size <=
			(free - header_len)) { // we must be able to fit data + header
			break;
		}
	}

	if (header != NULL) {
		return alloc_block(size, header);
	} else {
		return alloc_new(size);
	}
}

void *krealloc(void *src, size_t dst_len)
{
	struct page_header *header;
	size_t src_len;
	void *dst;

	// realloc of 0 means free pointer
	if (dst_len == 0) {
		kfree(src);
		return NULL;
	}

	// NULL src means allocate ptr
	if (src == NULL) {
		dst = kalloc(dst_len);
		return dst;
	}

	header = get_header(src);

	if (header == NULL)
		return NULL;

	src_len = header->used;

	if (src_len == 0)
		return NULL;

	dst = kalloc(dst_len);

	if (dst == NULL)
		return NULL; // allocation failed

	if (dst_len < src_len)
		src_len = dst_len;

	memcpy(dst, src, src_len);
	kfree(src);

	return dst;
}

void kfree(void *ptr)
{
	struct page_header *header;

	if (ptr == NULL)
		return;

	header = get_header(ptr);

	if (header == NULL)
		return;

	header->free += header->used;
	header->used = 0;

	struct page_header *neighbor;

	// merge left
	for (neighbor = header->prev; neighbor != NULL; neighbor = neighbor->prev) {
		if (neighbor->node_number != header->node_number)
			break;
		if (neighbor->used && header->used)
			break;
		neighbor->free += header->free + header_len;
		neighbor->next = header->next;
		header = neighbor;
	}

	// merge right
	for (neighbor = header->next; neighbor != NULL; neighbor = neighbor->next) {
		if (neighbor->node_number != header->node_number)
			break;
		if (neighbor->used)
			break;
		header->free += neighbor->free + header_len;
		header->next = neighbor->next;
	}

	if ((header->next == NULL ||
		 header->next->node_number != header->node_number) &&
		(header->prev == NULL ||
		 header->prev->node_number != header->node_number) &&
		header->used == 0) {
		if (header->next)
			header->next->prev = header->prev;
		if (header->prev)
			header->prev->next = header->next;
		kfree_pages(header);
	}
}
