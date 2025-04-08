#include <lib.h>
#include <comus/memory.h>
#include <comus/asm.h>

#include "physalloc.h"

extern char kernel_start;
extern char kernel_end;
#define kaddr(addr) ((uintptr_t)(&addr))

// between memory_start and kernel_start will be the bitmap
static uintptr_t memory_start = 0;

static uint64_t *bitmap;
static uint64_t total_memory;
static uint64_t free_memory;
static uint64_t page_count;
static uint64_t segment_count;
struct memory_segment *page_start;

static int n_pages(const struct memory_segment *m)
{
	return m->len / PAGE_SIZE;
}

static void *page_at(int i)
{
	int cur_page = 0;
	for (uint64_t idx = 0; idx < segment_count; idx++) {
		const struct memory_segment *m = page_start;
		int pages = n_pages(m);
		if (i - cur_page < pages) {
			return (void *)(m->addr + (PAGE_SIZE * (i - cur_page)));
		}
		cur_page += pages;
	}
	return NULL;
}

static long page_idx(void *page)
{
	uintptr_t addr = (uintptr_t)page;
	int cur_page = 0;
	for (uint64_t idx = 0; idx < segment_count; idx++) {
		const struct memory_segment *m = page_start;
		if ((uintptr_t)m + m->len > addr) {
			return cur_page + ((addr - m->addr) / PAGE_SIZE);
		}
		cur_page += n_pages(m);
	}
	return -1;
}

static inline bool bitmap_get(int i)
{
	return (bitmap[i / 64] >> i % 64) & 1;
}

static inline void bitmap_set(int i, bool v)
{
	if (v)
		free_memory -= PAGE_SIZE;
	else
		free_memory += PAGE_SIZE;
	int idx = i / 64;
	bitmap[idx] &= ~(1 << i % 64);
	bitmap[idx] |= (v << i % 64);
}

void *alloc_phys_page(void)
{
	return alloc_phys_pages(1);
}

void *alloc_phys_pages(int pages)
{
	if (pages < 1)
		return NULL;

	int n_contiguous = 0;
	int free_region_start = 0;
	for (uint64_t i = 0; i < page_count; i++) {
		bool free = !bitmap_get(i);

		if (free) {
			if (n_contiguous == 0)
				free_region_start = i;
			n_contiguous++;
			if (n_contiguous == pages) {
				for (int j = 0; j < pages; j++)
					bitmap_set(free_region_start + j, true);
				return page_at(free_region_start);
			}
		} else
			n_contiguous = 0;
	}

	return NULL;
}

void free_phys_page(void *ptr)
{
	free_phys_pages(ptr, 1);
}

void free_phys_pages(void *ptr, int pages)
{
	long idx = page_idx(ptr);
	if (idx == -1)
		return;

	for (int i = 0; i < pages; i++)
		bitmap_set(idx + pages, false);
}

static bool segment_invalid(const struct memory_segment *segment)
{
	if (segment->addr < kaddr(kernel_start))
		return true;
	if (segment->addr + segment->len < memory_start)
		return true;
	if (segment->addr + segment->len < kaddr(kernel_start))
		return true;
	return false;
}

static struct memory_segment clamp_segment(const struct memory_segment *segment)
{
	uint64_t length = segment->len;
	uintptr_t addr = segment->addr;

	uintptr_t start;
	if (memory_start)
		start = memory_start;
	else
		start = kaddr(kernel_end);

	if (segment->addr < start) {
		addr = start;
		length -= addr - segment->addr;
	} else {
		addr = segment->addr;
	}

	struct memory_segment temp;
	temp.len = length;
	temp.addr = addr;

	return temp;
}

static uintptr_t page_align(uintptr_t ptr)
{
	return (ptr + PAGE_SIZE - 1) / PAGE_SIZE * PAGE_SIZE;
}

void physalloc_init(struct memory_map *map)
{
	bitmap = NULL;
	total_memory = 0;
	free_memory = 0;
	page_count = 0;
	page_start = NULL;

	segment_count = 0;

	for (uint32_t i = 0; i < map->entry_count; i++) {
		struct memory_segment *segment = &map->entries[i];

		if (segment_invalid(segment))
			continue;

		struct memory_segment temp = clamp_segment(segment);
		page_count += n_pages(&temp);
		segment_count++;
	}

	long bitmap_pages = (page_count / 64 / PAGE_SIZE) + 1;
	long bitmap_size = bitmap_pages * PAGE_SIZE;
	bitmap = (uint64_t *)page_align(kaddr(kernel_end));

	long page_area_size = segment_count * sizeof(struct memory_segment);
	char *page_area_addr = (char *)bitmap + bitmap_size;
	page_area_addr = (char *)page_align((uintptr_t)page_area_addr);

	memory_start = page_align((uintptr_t)page_area_addr + page_area_size);

	bitmap = kmapaddr(bitmap, bitmap_size);
	memset(bitmap, 0, bitmap_size);
	page_area_addr = kmapaddr(page_area_addr, page_area_size);
	memset(page_area_addr, 0, page_area_size);

	page_start = (struct memory_segment *)page_area_addr;

	struct memory_segment *area = page_start;

	for (uint32_t i = 0; i < map->entry_count; i++) {
		struct memory_segment *segment = &map->entries[i];

		if (segment_invalid(segment))
			continue;

		struct memory_segment temp = clamp_segment(segment);
		*area = temp;
		area++;
	}

	total_memory = page_count * PAGE_SIZE;
	page_count -= bitmap_pages;
	free_memory = page_count * PAGE_SIZE;

	char buf[20];
	kprintf("\nMEMORY USAGE\n");
	kprintf("mem total: %s\n", btoa(memory_total(), buf));
	kprintf("mem free:  %s\n", btoa(memory_free(), buf));
	kprintf("mem used:  %s\n\n", btoa(memory_used(), buf));
}

uint64_t memory_total(void)
{
	return total_memory;
}

uint64_t memory_free(void)
{
	return free_memory;
}

uint64_t memory_used(void)
{
	return total_memory - free_memory;
}
