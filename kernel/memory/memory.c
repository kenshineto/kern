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

void kunmapaddr(const void *virt)
{
	mem_unmapaddr(kernel_mem_ctx, virt);
}

void *kget_phys(const void *virt)
{
	return mem_get_phys(kernel_mem_ctx, virt);
}

void *kalloc_page(void)
{
	return mem_alloc_page(kernel_mem_ctx, F_PRESENT | F_WRITEABLE);
}

void *kalloc_pages(size_t count)
{
	return mem_alloc_pages(kernel_mem_ctx, count, F_PRESENT | F_WRITEABLE);
}

void kfree_pages(const void *ptr)
{
	mem_free_pages(kernel_mem_ctx, ptr);
}

mem_ctx_t mem_ctx_alloc(void)
{
	mem_ctx_t ctx = user_mem_ctx_next;
	if (ctx == NULL)
		return NULL;

	if ((ctx->pml4 = pgdir_alloc()) == NULL)
		return NULL;
	virtaddr_init(&ctx->virtctx);

	user_mem_ctx_next = ctx->prev;
	if (ctx->prev)
		ctx->prev->next = NULL;
	ctx->prev = NULL;

	return ctx;
}

mem_ctx_t mem_ctx_clone(const mem_ctx_t old, bool cow)
{
	mem_ctx_t new;

	assert(old != NULL, "memory context is null");
	assert(old->pml4 != NULL, "pgdir is null");

	new = user_mem_ctx_next;
	if (new == NULL)
		return NULL;

	if ((new->pml4 = pgdir_clone(old->pml4, cow)) == NULL)
		return NULL;

	if (virtaddr_clone(&old->virtctx, &new->virtctx)) {
		pgdir_free(new->pml4);
		return NULL;
	}

	user_mem_ctx_next = new->prev;
	if (new->prev)
		new->prev->next = NULL;
	new->prev = NULL;

	return new;
}

void mem_ctx_free(mem_ctx_t ctx)
{
	assert(ctx != NULL, "memory context is null");
	assert(ctx->pml4 != NULL, "pgdir is null");

	pgdir_free(ctx->pml4);
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
	assert(ctx != NULL, "memory context is null");
	assert(ctx->pml4 != NULL, "pgdir is null");

	__asm__ volatile("mov %0, %%cr3" ::"r"(ctx->pml4) : "memory");
}

volatile void *mem_ctx_pgdir(mem_ctx_t ctx)
{
	assert(ctx != NULL, "memory context is null");
	assert(ctx->pml4 != NULL, "pgdir is null");

	return ctx->pml4;
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
