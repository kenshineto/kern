#include <comus/memory.h>
#include <comus/asm.h>
#include <comus/mboot.h>
#include <comus/efi.h>
#include <comus/limits.h>
#include <lib.h>

#include "memory.h"
#include "paging.h"
#include "virtalloc.h"
#include "physalloc.h"

// kernel memory context
mem_ctx_t kernel_mem_ctx;
static struct mem_ctx_s _kernel_mem_ctx;

// kernel page tables
extern volatile char kernel_pml4[];

// user space memory contexts
static struct mem_ctx_s user_mem_ctx[N_PROCS];
static struct mem_ctx_s *user_mem_ctx_next = NULL;

void *kmapaddr(void *phys, void *virt, size_t len, unsigned int flags)
{
	return mem_mapaddr(kernel_mem_ctx, phys, virt, len, flags);
}

void kunmapaddr(void *virt)
{
	mem_unmapaddr(kernel_mem_ctx, virt);
}

void *kalloc_page(void)
{
	return mem_alloc_page(kernel_mem_ctx, F_PRESENT | F_WRITEABLE, false);
}

void *kalloc_pages(size_t count)
{
	return mem_alloc_pages(kernel_mem_ctx, count, F_PRESENT | F_WRITEABLE,
						   false);
}

void kfree_pages(void *ptr)
{
	mem_free_pages(kernel_mem_ctx, ptr);
}

int kload_page(void *virt)
{
	return mem_load_page(kernel_mem_ctx, virt);
}

mem_ctx_t mem_ctx_alloc(void)
{
	mem_ctx_t ctx = user_mem_ctx_next;
	if (ctx == NULL)
		return NULL;

	if ((ctx->pml4 = pml4_alloc()) == NULL)
		return NULL;
	virtaddr_init(&ctx->virtctx);

	user_mem_ctx_next = ctx->prev;
	if (ctx->prev)
		ctx->prev->next = NULL;
	ctx->prev = NULL;

	return ctx;
}

mem_ctx_t mem_ctx_clone(mem_ctx_t ctx, bool cow)
{
	(void)ctx;
	(void)cow;

	panic("not yet implemented");
}

void mem_ctx_free(mem_ctx_t ctx)
{
	pml4_free(ctx->pml4);
	virtaddr_cleanup(&ctx->virtctx);

	if (user_mem_ctx_next == NULL) {
		user_mem_ctx_next = ctx;
	} else {
		user_mem_ctx_next->next = ctx;
		ctx->prev = user_mem_ctx_next;
		user_mem_ctx_next = ctx;
	}
}

void mem_ctx_switch(mem_ctx_t ctx)
{
	__asm__ volatile("mov %0, %%cr3" ::"r"(ctx->pml4) : "memory");
}

void memory_init(void)
{
	struct memory_map mmap;
	if (mboot_get_mmap(&mmap))
		if (efi_get_mmap(&mmap))
			panic("failed to load memory map");

	kernel_mem_ctx = &_kernel_mem_ctx;
	kernel_mem_ctx->pml4 = kernel_pml4;

	cli();
	paging_init();
	virtaddr_init(&kernel_mem_ctx->virtctx);
	physalloc_init(&mmap);
	sti();

	// identiy map EFI functions
	for (size_t i = 0; i < mmap.entry_count; i++) {
		struct memory_segment *seg = &mmap.entries[i];
		if (seg->type != SEG_TYPE_EFI)
			continue;
		kmapaddr((void *)seg->addr, (void *)seg->addr, seg->len, F_WRITEABLE);
	}

	// setup user mem ctx linked list
	for (size_t i = 0; i < N_PROCS; i++) {
		struct mem_ctx_s *ctx = &user_mem_ctx[i];
		ctx->next = NULL;
		ctx->prev = NULL;

		if (user_mem_ctx_next == NULL) {
			user_mem_ctx_next = ctx;
			continue;
		}

		user_mem_ctx_next->next = ctx;
		ctx->prev = user_mem_ctx_next;
		user_mem_ctx_next = ctx;
	}
}
