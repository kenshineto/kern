#include <lib.h>
#include <comus/memory.h>

#define MAGIC 0xBEEFCAFE

struct page_header {
	size_t len;
	uint64_t magic;
};

static struct page_header *get_header(void *ptr)
{
	struct page_header *header =
		(struct page_header *)((uintptr_t)ptr - PAGE_SIZE);

	if (header->magic != MAGIC)
		return NULL; // invalid pointer

	return header;
}

void *kalloc(size_t size)
{
	struct page_header *header;
	size_t pages;

	pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
	header = kalloc_pages(pages + 1);
	if (header == NULL)
		return NULL;

	header->magic = MAGIC;
	header->len = size;

	return (char *)header + PAGE_SIZE;
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

	src_len = header->len;

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

	kfree_pages(header);
}
