#include <lib.h>
#include <comus/memory.h>
#include <comus/asm.h>

#include "physalloc.h"

extern char kernel_start[];
extern char kernel_end[];

// between memory_start and kernel_start will be the bitmap
static uintptr_t memory_start = 0;

static uint64_t *bitmap = NULL;
static uint64_t total_memory;
static uint64_t free_memory;
static uint64_t page_count;
static uint64_t segment_count;
struct memory_map phys_mmap;
struct memory_segment *page_start = NULL;

static const char *segment_type_str[] = {
	[SEG_TYPE_FREE] = "Free",			[SEG_TYPE_RESERVED] = "Reserved",
	[SEG_TYPE_ACPI] = "ACPI Reserved",	[SEG_TYPE_HIBERNATION] = "Hibernation",
	[SEG_TYPE_DEFECTIVE] = "Defective", [SEG_TYPE_EFI] = "EFI Reserved",
};

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
		if (addr < m->addr) {
			return -1;
		}
		if ((uintptr_t)m + m->len > addr) {
			return cur_page + ((addr - m->addr) / PAGE_SIZE);
		}
		cur_page += n_pages(m);
	}
	return -1;
}

static inline bool bitmap_get(size_t i)
{
	return (bitmap[i / 64] >> i % 64) & 1;
}

static inline void bitmap_set(size_t i, bool v)
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
	return alloc_phys_pages_exact(1);
}

void *alloc_phys_pages_exact(size_t pages)
{
	if (pages < 1)
		return NULL;

	if (bitmap == NULL || page_start == NULL) {
		// temporary bump allocator
		void *addr = (void *)memory_start;
		assert(pages == 1,
			   "caller expects more pages, but is only getting one");
		memory_start += PAGE_SIZE;
		return addr;
	}

	size_t n_contiguous = 0;
	size_t free_region_start = 0;
	for (size_t i = 0; i < page_count; i++) {
		bool free = !bitmap_get(i);

		if (free) {
			if (n_contiguous == 0)
				free_region_start = i;
			n_contiguous++;
			if (n_contiguous == pages) {
				for (size_t j = 0; j < pages; j++)
					bitmap_set(free_region_start + j, true);
				return page_at(free_region_start);
			}
		} else
			n_contiguous = 0;
	}

	return NULL;
}

struct phys_page_slice alloc_phys_page_withextra(size_t max_pages)
{
	if (max_pages == 0)
		return PHYS_PAGE_SLICE_NULL;

	for (size_t i = 0; i < page_count; i++) {
		const bool free = !bitmap_get(i);
		if (!free)
			continue;

		// now allocated
		bitmap_set(i, true);

		// found at least one page, guaranteed to return valid slice at this
		// point
		struct phys_page_slice out = {
			.pagestart = page_at(i),
			.num_pages = 1,
		};

		// add some extra pages if possible
		for (; out.num_pages < MIN(page_count - i, max_pages);
			 ++out.num_pages) {
			// early return if max_pages isn't available
			if (bitmap_get(i + out.num_pages)) {
				return out;
			}
			bitmap_set(i + out.num_pages, true);
		}

		return out;
	}

	// only reachable if there is not a single free page in the bitmap
	return PHYS_PAGE_SLICE_NULL;
}

void free_phys_page(void *ptr)
{
	free_phys_pages(ptr, 1);
}

void free_phys_pages_slice(struct phys_page_slice slice)
{
	free_phys_pages(slice.pagestart, slice.num_pages);
}

void free_phys_pages(void *ptr, size_t pages)
{
	if (ptr == NULL)
		return;

	long idx = page_idx(ptr);
	if (idx == -1)
		return;

	for (size_t i = 0; i < pages; i++)
		bitmap_set(idx + i, false);
}

static bool segment_invalid(const struct memory_segment *segment)
{
	if (segment->len < 1)
		return true;
	if (segment->type != SEG_TYPE_FREE)
		return true;
	if (segment->addr < (uintptr_t) kernel_start)
		return true;
	if (segment->addr + segment->len < memory_start)
		return true;
	if (segment->addr + segment->len < (uintptr_t) kernel_start)
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
		start = (uintptr_t) kernel_end;

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
	phys_mmap = *map;

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
	bitmap = (uint64_t *)page_align((uintptr_t)kernel_end);

	long page_area_size = segment_count * sizeof(struct memory_segment);
	char *page_area_addr = (char *)bitmap + bitmap_size;
	page_area_addr = (char *)page_align((uintptr_t)page_area_addr);

	memory_start = page_align((uintptr_t)page_area_addr + page_area_size);

	bitmap = kmapaddr(bitmap, NULL, bitmap_size, F_WRITEABLE);
	memset(bitmap, 0, bitmap_size);
	page_area_addr =
		kmapaddr(page_area_addr, NULL, page_area_size, F_WRITEABLE);
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

void memory_report(void)
{
	char buf[20];

	kprintf("MEMORY MAP\n");
	for (uint32_t i = 0; i < phys_mmap.entry_count; i++) {
		struct memory_segment *seg;
		seg = &phys_mmap.entries[i];
		kprintf("ADDR: %16p  LEN: %4s  TYPE: %s\n", (void *)seg->addr,
				btoa(seg->len, buf), segment_type_str[seg->type]);
	}

	kprintf("\nMEMORY USAGE\n");
	kprintf("mem total: %s\n", btoa(memory_total(), buf));
	kprintf("mem free:  %s\n", btoa(memory_free(), buf));
	kprintf("mem used:  %s\n\n", btoa(memory_used(), buf));
}
