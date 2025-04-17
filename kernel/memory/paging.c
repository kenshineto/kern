#include <lib.h>
#include <comus/memory.h>

#include <stddef.h>
#include <stdint.h>

#include "virtalloc.h"
#include "physalloc.h"
#include "paging.h"
#include "memory.h"

// PAGE MAP LEVEL 4 ENTRY
struct pml4e {
	uint64_t flags : 6;
	uint64_t : 6;
	uint64_t address : 40;
	uint64_t : 11;
	uint64_t execute_disable : 1;
};

// PAGE DIRECTORY POINTER TABLE ENTRY
struct pdpte {
	uint64_t flags : 6;
	uint64_t : 1;
	uint64_t page_size : 1;
	uint64_t : 4;
	uint64_t address : 40;
	uint64_t : 11;
	uint64_t execute_disable : 1;
};

// PAGE DIRECTORY ENTRY
struct pde {
	uint64_t flags : 6;
	uint64_t : 1;
	uint64_t page_size : 1;
	uint64_t : 4;
	uint64_t address : 40;
	uint64_t : 11;
	uint64_t execute_disable : 1;
};

// PAGE TABLE ENTRY
struct pte {
	uint64_t flags : 9;
	uint64_t loaded : 1;
	uint64_t : 2;
	uint64_t address : 40;
	uint64_t : 7;
	uint64_t protection_key : 4;
	uint64_t execute_disable : 1;
};

// bss segment, can write to
extern volatile struct pml4e kernel_pml4[512];
extern volatile struct pdpte kernel_pdpt_0[512];
extern volatile struct pde kernel_pd_0[512];
extern volatile struct pte bootstrap_pt[512];
extern volatile struct pte
	paging_pt[512]; // paging_pt should NEVER be outside of this file, NEVER i say

// paged address to read page tables
// the structures are not gurenteed to be ident mapped
// map them here with map_<type>(phys_addr) before useing structures
void volatile *addr_mapped = (void *)(uintptr_t)0x204000;
static volatile struct pml4e *pml4_mapped = (void *)(uintptr_t)0x200000;
static volatile struct pdpte *pdpt_mapped = (void *)(uintptr_t)0x201000;
static volatile struct pde *pd_mapped = (void *)(uintptr_t)0x202000;
static volatile struct pte *pt_mapped = (void *)(uintptr_t)0x203000;

static inline void invlpg(volatile void *addr)
{
	__asm__ volatile("invlpg (%0)" ::"r"(addr) : "memory");
}

static void load_addr(volatile void *phys_addr)
{
	static volatile struct pte *pt = &paging_pt[4];
	pt->address = (uint64_t)phys_addr >> 12;
	pt->flags = F_PRESENT | F_WRITEABLE;
	invlpg(addr_mapped);
}

static void load_pml4(volatile void *phys)
{
	static volatile struct pte *pt = &paging_pt[0];
	if ((uint64_t)phys >> 12 == pt->address)
		return;
	pt->address = (uint64_t)phys >> 12;
	pt->flags = F_PRESENT | F_WRITEABLE;
	invlpg(pml4_mapped);
}

static void load_pdpt(volatile void *phys)
{
	static volatile struct pte *pt = &paging_pt[1];
	if ((uint64_t)phys >> 12 == pt->address)
		return;
	pt->address = (uint64_t)phys >> 12;
	pt->flags = F_PRESENT | F_WRITEABLE;
	invlpg(pdpt_mapped);
}

static void load_pd(volatile void *phys)
{
	static volatile struct pte *pt = &paging_pt[2];
	if ((uint64_t)phys >> 12 == pt->address)
		return;
	pt->address = (uint64_t)phys >> 12;
	pt->flags = F_PRESENT | F_WRITEABLE;
	invlpg(pd_mapped);
}

static void load_pt(volatile void *phys)
{
	static volatile struct pte *pt = &paging_pt[3];
	if ((uint64_t)phys >> 12 == pt->address)
		return;
	pt->address = (uint64_t)phys >> 12;
	pt->flags = F_PRESENT | F_WRITEABLE;
	invlpg(pt_mapped);
}

#define PAG_SUCCESS 0
#define PAG_CANNOT_ALLOC 1
#define PAG_NOT_PRESENT 2

static int select_pdpt(void *virt, unsigned int flags,
					   volatile struct pml4e *root, volatile struct pdpte **res,
					   bool create)
{
	load_pml4(root);
	uint64_t offset = (uint64_t)virt >> 39;
	volatile struct pml4e *pml4e = &pml4_mapped[offset];
	if (!(pml4e->flags & F_PRESENT)) {
		if (!create) {
			return PAG_NOT_PRESENT;
		}
		void *new_page = alloc_phys_page();
		if (new_page == NULL) {
			return PAG_CANNOT_ALLOC;
		}
		load_addr(new_page);
		memsetv(addr_mapped, 0, PAGE_SIZE);
		pml4e->address = ((uint64_t)new_page) >> 12;
		pml4e->flags = F_PRESENT;
	}
	if (flags)
		pml4e->flags = F_PRESENT | flags;
	*res = (volatile struct pdpte *)(uintptr_t)(pml4e->address << 12);
	return PAG_SUCCESS;
}

static int select_pd(void *virt, unsigned int flags,
					 volatile struct pdpte *pdpt, volatile struct pde **res,
					 bool create)
{
	load_pdpt(pdpt);
	uint64_t offset = ((uint64_t)virt >> 30) & 0x1ff;
	volatile struct pdpte *pdpte = &pdpt_mapped[offset];
	if (!(pdpte->flags & F_PRESENT)) {
		if (!create) {
			return PAG_NOT_PRESENT;
		}
		void *new_page = alloc_phys_page();
		if (new_page == NULL) {
			return PAG_CANNOT_ALLOC;
		}
		load_addr(new_page);
		memsetv(addr_mapped, 0, PAGE_SIZE);
		pdpte->address = ((uint64_t)new_page) >> 12;
		pdpte->flags = F_PRESENT;
	}
	if (flags)
		pdpte->flags = F_PRESENT | flags;
	*res = (volatile struct pde *)(uintptr_t)(pdpte->address << 12);
	return PAG_SUCCESS;
}

static int select_pt(void *virt, unsigned int flags, volatile struct pde *pd,
					 volatile struct pte **res, bool create)
{
	load_pd(pd);
	uint64_t offset = ((uint64_t)virt >> 21) & 0x1ff;
	volatile struct pde *pde = &pd_mapped[offset];
	if (!(pde->flags & F_PRESENT)) {
		if (!create) {
			return PAG_NOT_PRESENT;
		}
		void *new_page = alloc_phys_page();
		if (new_page == NULL) {
			return PAG_CANNOT_ALLOC;
		}
		load_addr(new_page);
		memsetv(addr_mapped, 0, PAGE_SIZE);
		pde->address = ((uint64_t)new_page) >> 12;
		pde->flags = F_PRESENT;
	}
	if (flags)
		pde->flags = F_PRESENT | flags;
	*res = (volatile struct pte *)(uintptr_t)(pde->address << 12);
	return PAG_SUCCESS;
}

static void select_page(void *virt, volatile struct pte *pt,
						volatile struct pte **res)
{
	load_pt(pt);
	uint64_t offset = ((uint64_t)virt >> 12) & 0x1ff;
	volatile struct pte *page = &pt_mapped[offset];
	*res = page;
	return;
}

static inline void try_unmap_pml4(void)
{
	for (int i = 0; i < 512; i++) {
		if (pml4_mapped[i].flags & F_PRESENT)
			return;
	}
	for (int i = 0; i < 512; i++) {
		if (pml4_mapped[i].address) {
			void *addr = (void *)(uintptr_t)(pml4_mapped[i].address << 12);
			free_phys_page(addr);
		}
	}
}

static inline void try_unmap_pdpt(void)
{
	for (int i = 0; i < 512; i++) {
		if (pdpt_mapped[i].flags & F_PRESENT)
			return;
	}
	for (int i = 0; i < 512; i++) {
		if (pdpt_mapped[i].address) {
			void *addr = (void *)(uintptr_t)(pdpt_mapped[i].address << 12);
			free_phys_page(addr);
		}
	}
	try_unmap_pml4();
}

static inline void try_unmap_pd(void)
{
	for (int i = 0; i < 512; i++) {
		if (pd_mapped[i].flags & F_PRESENT)
			return;
	}
	for (int i = 0; i < 512; i++) {
		if (pd_mapped[i].address) {
			void *addr = (void *)(uintptr_t)(pd_mapped[i].address << 12);
			free_phys_page(addr);
		}
	}
	try_unmap_pdpt();
}

static inline void try_unmap_pt(void)
{
	for (int i = 0; i < 512; i++) {
		if (pt_mapped[i].flags & F_PRESENT)
			return;
	}
	for (int i = 0; i < 512; i++) {
		if (pt_mapped[i].address) {
			void *addr = (void *)(uintptr_t)(pt_mapped[i].address << 12);
			free_phys_page(addr);
		}
	}
	try_unmap_pd();
}

static void unmap_page(volatile struct pml4e *root, void *virt)
{
	volatile struct pdpte *pdpt;
	volatile struct pde *pd;
	volatile struct pte *pt;
	volatile struct pte *page;

	unsigned int df = 0;

	if (select_pdpt(virt, df, root, &pdpt, false))
		return;

	if (select_pd(virt, df, pdpt, &pd, false))
		return;

	if (select_pt(virt, df, pd, &pt, false))
		return;

	select_page(virt, pt, &page);

	page->address = 0;
	page->flags = 0;
	page->loaded = 0;

	try_unmap_pt();

	invlpg(virt);

	return;
}

static void unmap_pages(volatile struct pml4e *root, void *virt_start,
						long page_count)
{
	uint64_t pml4_o = -1, pdpt_o = -1, pd_o = -1;

	uint64_t pml4_n, pdpt_n, pd_n;

	volatile struct pdpte *pdpt = NULL;
	volatile struct pde *pd = NULL;
	volatile struct pte *pt = NULL;
	volatile struct pte *page = NULL;

	unsigned int df = 0;

	void *virt;

	for (long i = 0; i < page_count; i++) {
		virt = (char *)virt_start + (i * PAGE_SIZE);

		pml4_n = (uint64_t)virt >> 39;
		pdpt_n = ((uint64_t)virt >> 30) & 0x1ff;
		pd_n = ((uint64_t)virt >> 21) & 0x1ff;

		if (pdpt == NULL || pml4_o != pml4_n) {
			if (select_pdpt(virt, df, root, &pdpt, false))
				continue;
			pml4_o = pml4_n;
		}

		if (pd == NULL || pdpt_o != pdpt_n) {
			if (select_pd(virt, df, pdpt, &pd, false))
				continue;
			pdpt_o = pdpt_n;
		}

		if (pt == NULL || pd_o != pd_n) {
			if (pt) {
				try_unmap_pt();
			}
			if (select_pt(virt, df, pd, &pt, false))
				continue;
			pd_o = pd_n;
		}

		select_page(virt, pt, &page);

		page->address = 0;
		page->flags = 0;
		page->loaded = 0;
	}

	if (pt != NULL)
		try_unmap_pt();

	return;
}

static volatile struct pte *get_page(volatile struct pml4e *root, void *virt)
{
	volatile struct pdpte *pdpt;
	volatile struct pde *pd;
	volatile struct pte *pt;
	volatile struct pte *page;

	unsigned int df = 0;

	if (select_pdpt(virt, df, root, &pdpt, false))
		return NULL;

	if (select_pd(virt, df, pdpt, &pd, false))
		return NULL;

	if (select_pt(virt, df, pd, &pt, false))
		return NULL;

	select_page(virt, pt, &page);

	return page;
}

static int map_page(volatile struct pml4e *root, void *virt, void *phys,
					unsigned int flags)
{
	volatile struct pdpte *pdpt;
	volatile struct pde *pd;
	volatile struct pte *pt;
	volatile struct pte *page;

	unsigned int df = F_WRITEABLE;

	if (phys == NULL)
		df = 0; // alloc page on fault

	if (select_pdpt(virt, df, root, &pdpt, true))
		return 1;

	if (select_pd(virt, df, pdpt, &pd, true))
		return 1;

	if (select_pt(virt, df, pd, &pt, true))
		return 1;

	select_page(virt, pt, &page);

	if (phys) {
		page->flags = F_PRESENT | flags;
		page->address = (uint64_t)phys >> 12;
		page->loaded = 1;
		invlpg(virt);
	} else {
		page->flags = flags;
		page->address = 0;
		page->loaded = 0;
	}

	return 0;
}

static int map_pages(volatile struct pml4e *root, void *virt_start,
					 void *phys_start, unsigned int flags, long page_count)
{
	uint64_t pml4_o = -1, pdpt_o = -1, pd_o = -1;

	uint64_t pml4_n, pdpt_n, pd_n;

	volatile struct pdpte *pdpt = NULL;
	volatile struct pde *pd = NULL;
	volatile struct pte *pt = NULL;
	volatile struct pte *page = NULL;

	void *virt, *phys;

	unsigned int df = F_WRITEABLE;

	if (phys_start == NULL)
		df = 0; // alloc page on fault

	long i;
	for (i = 0; i < page_count; i++) {
		virt = (char *)virt_start + (i * PAGE_SIZE);
		phys = (char *)phys_start + (i * PAGE_SIZE);

		pml4_n = (uint64_t)virt >> 39;
		pdpt_n = ((uint64_t)virt >> 30) & 0x1ff;
		pd_n = ((uint64_t)virt >> 21) & 0x1ff;

		if (pdpt == NULL || pml4_o != pml4_n) {
			if (select_pdpt(virt, df, root, &pdpt, true))
				goto failed;
			pml4_o = pml4_n;
		}

		if (pd == NULL || pdpt_o != pdpt_n) {
			if (select_pd(virt, df, pdpt, &pd, true))
				goto failed;
			pdpt_o = pdpt_n;
		}

		if (pt == NULL || pd_o != pd_n) {
			if (select_pt(virt, df, pd, &pt, true))
				goto failed;
			pd_o = pd_n;
		}

		select_page(virt, pt, &page);

		if (phys_start) {
			page->flags = F_PRESENT | flags;
			page->address = (uint64_t)phys >> 12;
			page->loaded = 1;
		} else {
			page->flags = flags;
			page->address = 0;
			page->loaded = 0;
		}

		if (flags & F_GLOBAL)
			invlpg(virt);
	}

	__asm__ volatile("mov %cr3, %rax; mov %rax, %cr3;");

	return 0;

failed:

	unmap_pages(root, virt, i);

	return 1;
}

void paging_init(void)
{
	kernel_pml4[0].flags = F_PRESENT | F_WRITEABLE;
	kernel_pml4[0].address = (uint64_t)(&kernel_pdpt_0) >> 12;

	kernel_pdpt_0[0].flags = F_PRESENT | F_WRITEABLE;
	kernel_pdpt_0[0].address = (uint64_t)(&kernel_pd_0) >> 12;

	kernel_pd_0[1].flags = F_PRESENT | F_WRITEABLE;
	kernel_pd_0[1].address = (uint64_t)(&paging_pt) >> 12;
	kernel_pd_0[2].flags = F_PRESENT | F_WRITEABLE;
	kernel_pd_0[2].address = (uint64_t)(&bootstrap_pt) >> 12;

	memsetv(&paging_pt, 0, 4096);
	memsetv(&bootstrap_pt, 0, 4096);
}

static inline void *page_align(void *addr)
{
	uintptr_t a = (uintptr_t)addr;
	a /= PAGE_SIZE;
	a *= PAGE_SIZE;
	return (void *)a;
}

void *mem_mapaddr(mem_ctx_t ctx, void *phys, void *virt, size_t len,
				  unsigned int flags)
{
	TRACE("PHYS %16p VIRT %16p LEN %zu FLAGS %08x \n", phys, virt, len, flags);

	long pages;
	ptrdiff_t error;
	void *aligned_phys;

	// get length and physical page aligned address
	aligned_phys = page_align(phys);
	error = (char *)phys - (char *)aligned_phys;
	len += error;
	pages = len / PAGE_SIZE + 1;

	// get page aligned (or allocate) vitural address
	if (virt == NULL)
		virt = virtaddr_alloc(ctx->virtctx, pages);
	if (virt == NULL)
		return NULL;

	if (map_pages((volatile struct pml4e *)ctx->pml4, virt, aligned_phys,
				  F_WRITEABLE | flags, pages)) {
		virtaddr_free(ctx->virtctx, virt);
		return NULL;
	}

	return (char *)virt + error;
}

void mem_unmapaddr(mem_ctx_t ctx, void *virt)
{
	TRACE("VIRT %16p\n", virt);

	long pages = virtaddr_free(ctx->virtctx, virt);
	if (pages < 1)
		return;
	unmap_pages(kernel_pml4, virt, pages);
}

void *mem_alloc_page(mem_ctx_t ctx)
{
	return mem_alloc_pages(ctx, 1);
}

void *mem_alloc_pages(mem_ctx_t ctx, size_t count)
{
	TRACE("COUNT %zu\n", count);

	void *virt = virtaddr_alloc(ctx->virtctx, count);
	if (virt == NULL)
		return NULL;
	//void *phys = alloc_phys_pages(count);
	//if (phys == NULL) {
	//	virtaddr_free(virt);
	//	return NULL;
	//}
	if (map_pages((volatile struct pml4e *)ctx->pml4, virt,
				  //phys,
				  //F_WRITEABLE,
				  NULL, F_WRITEABLE, count)) {
		virtaddr_free(ctx->virtctx, virt);
		return NULL;
	}
	return virt;
}

void mem_free_pages(mem_ctx_t ctx, void *virt)
{
	TRACE("VIRT %16p\n", virt);

	long pages = virtaddr_free(ctx->virtctx, virt);
	if (pages == 1)
		unmap_page((volatile struct pml4e *)ctx->pml4, virt);
	else if (pages > 1)
		unmap_pages((volatile struct pml4e *)ctx->pml4, virt, pages);
}

int mem_load_page(mem_ctx_t ctx, void *virt_addr)
{
	TRACE("VIRT %16p\n", virt_addr);

	volatile struct pte *page =
		get_page((volatile struct pml4e *)ctx->pml4, virt_addr);
	if (page == NULL)
		return -1;
	if (page->loaded)
		return -1;
	void *phys = alloc_phys_page();
	if (phys == NULL)
		return -2;
	page->loaded = 1;
	page->address = (uint64_t)phys >> 12;
	page->flags |= F_PRESENT;
	invlpg(virt_addr);
	return 0;
}
