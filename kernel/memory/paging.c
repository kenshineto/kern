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
	uint64_t : 1; // ignored
	uint64_t : 1; // reserved
	uint64_t : 4; // ignored
	uint64_t address : 40;
	uint64_t : 11; // ignored
	uint64_t execute_disable : 1;
} __attribute__((packed));

// PAGE MAP LEVEL 4
struct pml4 {
	union {
		// pml4 metadata
		struct {
			uint64_t : 6; // flags
			uint64_t : 1; // ignored
			uint64_t : 1; // reserved
			uint64_t : 4; // ignored
			uint64_t : 40; // address
			uint64_t : 2; // ignored
			uint64_t count : 9; // ignored
			uint64_t : 1; // execute_disable
		};
		// entries
		struct pml4e entries[512];
	};
} __attribute__((packed));

// PAGE DIRECTORY POINTER TABLE ENTRY
struct pdpte {
	uint64_t flags : 6;
	uint64_t : 1; // ignored
	uint64_t page_size : 1;
	uint64_t : 4; // ignored
	uint64_t address : 40;
	uint64_t : 11; // ignored
	uint64_t execute_disable : 1;
} __attribute__((packed, aligned(8)));

// PAGE DIRECTORY POINTER TABLE
struct pdpt {
	union {
		// pdpt metadata
		struct {
			uint64_t : 6; // flags
			uint64_t : 1; // ignored
			uint64_t : 1; // page_size
			uint64_t : 4; // ignored
			uint64_t : 40; // address
			uint64_t : 2; // ignored
			uint64_t count : 9; // ignored
			uint64_t : 1; // execute_disable
		};
		// entries
		struct pdpte entries[512];
	};
} __attribute__((packed, aligned(4096)));

// PAGE DIRECTORY ENTRY
struct pde {
	uint64_t flags : 6;
	uint64_t : 1; // ignored
	uint64_t page_size : 1;
	uint64_t : 4; // ignored
	uint64_t address : 40;
	uint64_t : 11; // ignored
	uint64_t execute_disable : 1;
} __attribute__((packed, aligned(8)));

// PAGE DIRECTORY
struct pd {
	union {
		// pd metadata
		struct {
			uint64_t : 6; // flags
			uint64_t : 1; // ignored
			uint64_t : 1; // page_size
			uint64_t : 4; // ignored
			uint64_t : 40; // address
			uint64_t : 2; // ignored
			uint64_t count : 9; // ignored
			uint64_t : 1; // execute_disable
		};
		// entries
		struct pde entries[512];
	};
} __attribute__((packed, aligned(4096)));

// PAGE TABLE ENTRY
struct pte {
	uint64_t flags : 9;
	uint64_t loaded : 1; // ignored
	uint64_t : 2; // ignored
	uint64_t address : 40;
	uint64_t : 7; // ignored
	uint64_t protection_key : 4;
	uint64_t execute_disable : 1;
} __attribute__((packed, aligned(8)));

// PAGE TABLE
struct pt {
	union {
		// pt metadata
		struct {
			uint64_t : 9; // flags
			uint64_t : 1; // (loaded) ignored
			uint64_t count_low : 2; // ignored
			uint64_t : 40; // address
			uint64_t count_high : 7; // ignored
			uint64_t : 4; // protection_key
			uint64_t : 1; // execute_disable
		};
		// entries
		struct pte entries[512];
	};
} __attribute__((packed, aligned(4096)));

// bootstraping kernel paging structures
extern volatile struct pml4 kernel_pml4;
extern volatile struct pdpt kernel_pdpt_0;
extern volatile struct pd kernel_pd_0;
extern volatile struct pt kernel_pd_0_ents[N_IDENT_PTS];
extern volatile struct pd kernel_pd_1;
extern volatile struct pt
	paging_pt; // paging_pt should NEVER be outside of this file, NEVER i say

// paged address to read page tables
// the structures are not gurenteed to be ident mapped
// map them here with load_<type>(phys_addr) before useing structures
static volatile struct pml4 *pml4_mapped = (void *)(uintptr_t)0x40000000;
static volatile struct pdpt *pdpt_mapped = (void *)(uintptr_t)0x40001000;
static volatile struct pd *pd_mapped = (void *)(uintptr_t)0x40002000;
static volatile struct pt *pt_mapped = (void *)(uintptr_t)0x40003000;
static volatile void *addr_mapped = (void *)(uintptr_t)0x40004000;

// kernel start/end
extern char kernel_start[];
extern char kernel_end[];

static inline void invlpg(volatile void *addr)
{
	__asm__ volatile("invlpg (%0)" ::"r"(addr) : "memory");
}

static volatile struct pml4 *load_pml4(volatile void *phys)
{
	static volatile struct pte *pte = &paging_pt.entries[0];
	if ((uint64_t)phys >> 12 == pte->address)
		return pml4_mapped;
	pte->address = (uint64_t)phys >> 12;
	pte->flags = F_PRESENT | F_WRITEABLE;
	invlpg(pml4_mapped);
	return pml4_mapped;
}

static volatile struct pdpt *load_pdpt(volatile void *phys)
{
	static volatile struct pte *pte = &paging_pt.entries[1];
	if ((uint64_t)phys >> 12 == pte->address)
		return pdpt_mapped;
	pte->address = (uint64_t)phys >> 12;
	pte->flags = F_PRESENT | F_WRITEABLE;
	invlpg(pdpt_mapped);
	return pdpt_mapped;
}

static volatile struct pd *load_pd(volatile void *phys)
{
	static volatile struct pte *pte = &paging_pt.entries[2];
	if ((uint64_t)phys >> 12 == pte->address)
		return pd_mapped;
	pte->address = (uint64_t)phys >> 12;
	pte->flags = F_PRESENT | F_WRITEABLE;
	invlpg(pd_mapped);
	return pd_mapped;
}

static volatile struct pt *load_pt(volatile void *phys)
{
	static volatile struct pte *pte = &paging_pt.entries[3];
	if ((uint64_t)phys >> 12 == pte->address)
		return pt_mapped;
	pte->address = (uint64_t)phys >> 12;
	pte->flags = F_PRESENT | F_WRITEABLE;
	invlpg(pt_mapped);
	return pt_mapped;
}

static volatile void *load_addr(volatile void *phys_addr)
{
	static volatile struct pte *pte = &paging_pt.entries[4];
	pte->address = (uint64_t)phys_addr >> 12;
	pte->flags = F_PRESENT | F_WRITEABLE;
	invlpg(addr_mapped);
	return addr_mapped;
}

#define PAG_SUCCESS 0
#define PAG_CANNOT_ALLOC 1
#define PAG_NOT_PRESENT 2

static int select_pdpt(void *virt, unsigned int flags,
					   volatile struct pml4 *pml4, volatile struct pdpt **res,
					   bool create)
{
	load_pml4(pml4);
	uint64_t offset = (uint64_t)virt >> 39;
	volatile struct pml4e *pml4e = &pml4_mapped->entries[offset];
	if (!(pml4e->flags & F_PRESENT)) {
		if (!create)
			return PAG_NOT_PRESENT;
		void *new_page = alloc_phys_page();
		if (new_page == NULL)
			return PAG_CANNOT_ALLOC;
		load_addr(new_page);
		memsetv(addr_mapped, 0, PAGE_SIZE);
		pml4e->address = ((uint64_t)new_page) >> 12;
		pml4e->flags = F_PRESENT;
		// update count
		pml4_mapped->count++;
	}
	if (flags)
		pml4e->flags = F_PRESENT | flags;
	*res = (volatile struct pdpt *)(uintptr_t)(pml4e->address << 12);
	return PAG_SUCCESS;
}

static int select_pd(void *virt, unsigned int flags, volatile struct pdpt *pdpt,
					 volatile struct pd **res, bool create)
{
	load_pdpt(pdpt);
	uint64_t offset = ((uint64_t)virt >> 30) & 0x1ff;
	volatile struct pdpte *pdpte = &pdpt_mapped->entries[offset];
	if (!(pdpte->flags & F_PRESENT)) {
		if (!create)
			return PAG_NOT_PRESENT;
		void *new_page = alloc_phys_page();
		if (new_page == NULL)
			return PAG_CANNOT_ALLOC;
		load_addr(new_page);
		memsetv(addr_mapped, 0, PAGE_SIZE);
		pdpte->address = ((uint64_t)new_page) >> 12;
		pdpte->flags = F_PRESENT;
		// update count
		pdpt_mapped->count++;
	}
	if (flags)
		pdpte->flags = F_PRESENT | flags;
	*res = (volatile struct pd *)(uintptr_t)(pdpte->address << 12);
	return PAG_SUCCESS;
}

static int select_pt(void *virt, unsigned int flags, volatile struct pd *pd,
					 volatile struct pt **res, bool create)
{
	load_pd(pd);
	uint64_t offset = ((uint64_t)virt >> 21) & 0x1ff;
	volatile struct pde *pde = &pd_mapped->entries[offset];
	if (!(pde->flags & F_PRESENT)) {
		if (!create)
			return PAG_NOT_PRESENT;
		void *new_page = alloc_phys_page();
		if (new_page == NULL)
			return PAG_CANNOT_ALLOC;
		load_addr(new_page);
		memsetv(addr_mapped, 0, PAGE_SIZE);
		pde->address = ((uint64_t)new_page) >> 12;
		pde->flags = F_PRESENT;
		// update count
		pd_mapped->count++;
	}
	if (flags)
		pde->flags = F_PRESENT | flags;
	*res = (volatile struct pt *)(uintptr_t)(pde->address << 12);
	return PAG_SUCCESS;
}

static void select_page(void *virt, volatile struct pt *pt,
						volatile struct pte **res)
{
	load_pt(pt);
	uint64_t offset = ((uint64_t)virt >> 12) & 0x1ff;
	volatile struct pte *page = &pt_mapped->entries[offset];
	*res = page;
	return;
}

static void pte_free(volatile struct pte *vPTE)
{
	volatile struct pte *pPTE;

	if (!(vPTE->flags & F_PRESENT))
		return;

	pPTE = (volatile struct pte *)((uintptr_t)vPTE->address << 12);

	// allocated, free phys page
	free_phys_page((void*)(uintptr_t)pPTE);
}

static void pt_free(volatile struct pde *vPDE)
{
	volatile struct pt *pPT, *vPT;
	uint64_t count;

	if (!(vPDE->flags & F_PRESENT))
		return;

	pPT = (volatile struct pt *)((uintptr_t)vPDE->address << 12);
	vPT = load_pt(pPT);
	count = (vPT->count_high << 2) | vPT->count_low;

	if (count)
		for (uint64_t i = 0; i < 512; i++)
			pte_free(&vPT->entries[i]);
	free_phys_page((void*)(uintptr_t)pPT);
}

static void pd_free(volatile struct pdpte *vPDPTE)
{
	volatile struct pd *pPD, *vPD;
	uint64_t count;

	if (!(vPDPTE->flags & F_PRESENT))
		return;

	pPD = (volatile struct pd *)((uintptr_t)vPDPTE->address << 12);
	vPD = load_pd(pPD);
	count = vPD->count;

	if (count)
		for (uint64_t i = 0; i < 512; i++)
			pt_free(&vPD->entries[i]);
	free_phys_page((void*)(uintptr_t)pPD);
}

static void pdpt_free(volatile struct pml4e *vPML4E)
{
	volatile struct pdpt *pPDPT, *vPDPT;
	uint64_t count;

	if (!(vPML4E->flags & F_PRESENT))
		return;

	pPDPT = (volatile struct pdpt *)((uintptr_t)vPML4E->address << 12);
	vPDPT = load_pdpt(pPDPT);
	count = vPDPT->count;

	if (count)
		for (uint64_t i = 0; i < 512; i++)
			pd_free(&vPDPT->entries[i]);
	free_phys_page((void*)(uintptr_t)pPDPT);
}

void pml4_free(volatile void *pPML4)
{
	volatile struct pml4 *vPML4;
	uint64_t count;

	vPML4 = load_pml4(pPML4);
	count = vPML4->count;

	if (count)
		for (uint64_t i = 0; i < 512; i++)
			pdpt_free(&vPML4->entries[i]);
	free_phys_page((void*)(uintptr_t)pPML4);
}

static void unmap_page(volatile struct pml4 *pPML4, void *virt)
{
	volatile struct pdpt *pPDPT;
	volatile struct pd *pPD;
	volatile struct pt *pPT;
	volatile struct pte *vPTE;

	unsigned int df = 0;

	if (select_pdpt(virt, df, pPML4, &pPDPT, false))
		return;

	if (select_pd(virt, df, pPDPT, &pPD, false))
		return;

	if (select_pt(virt, df, pPD, &pPT, false))
		return;

	select_page(virt, pPT, &vPTE);

	// update counts in parent structures
	pml4_mapped->count--;
	pdpt_mapped->count--;
	pd_mapped->count--;

	// unmap page
	vPTE->address = 0;
	vPTE->flags = 0;
	vPTE->loaded = 0;

	invlpg(virt);

	return;
}

static void unmap_pages(volatile struct pml4 *pPML4, void *virt_start,
						long page_count)
{
	uint64_t pml4_o = -1, pdpt_o = -1, pd_o = -1;

	uint64_t pml4_n, pdpt_n, pd_n;

	volatile struct pdpt *pPDPT = NULL;
	volatile struct pd *pPD = NULL;
	volatile struct pt *pPT = NULL;
	volatile struct pte *vPTE = NULL;

	unsigned int df = 0;

	void *virt;

	for (long i = 0; i < page_count; i++) {
		virt = (char *)virt_start + (i * PAGE_SIZE);

		pml4_n = (uint64_t)virt >> 39;
		pdpt_n = ((uint64_t)virt >> 30) & 0x1ff;
		pd_n = ((uint64_t)virt >> 21) & 0x1ff;

		if (pPDPT == NULL || pml4_o != pml4_n) {
			if (select_pdpt(virt, df, pPML4, &pPDPT, false))
				continue;
			pml4_o = pml4_n;
		}

		if (pPD == NULL || pdpt_o != pdpt_n) {
			if (select_pd(virt, df, pPDPT, &pPD, false))
				continue;
			pdpt_o = pdpt_n;
		}

		if (pPT == NULL || pd_o != pd_n) {
			if (pPT) {
				//pt_try_free();
			}
			if (select_pt(virt, df, pPD, &pPT, false))
				continue;
			pd_o = pd_n;
		}

		select_page(virt, pPT, &vPTE);

		// update counts in parent structures
		pml4_mapped->count--;
		pdpt_mapped->count--;
		pd_mapped->count--;

		// unmap page
		vPTE->address = 0;
		vPTE->flags = 0;
		vPTE->loaded = 0;
	}

	//if (pt != NULL)
	//	pt_try_free();

	return;
}

static volatile struct pte *get_page(volatile struct pml4 *pPML4, void *virt)
{
	volatile struct pdpt *pPDPT;
	volatile struct pd *pPD;
	volatile struct pt *pPT;
	volatile struct pte *vPTE;

	unsigned int df = 0;

	if (select_pdpt(virt, df, pPML4, &pPDPT, false))
		return NULL;

	if (select_pd(virt, df, pPDPT, &pPD, false))
		return NULL;

	if (select_pt(virt, df, pPD, &pPT, false))
		return NULL;

	select_page(virt, pPT, &vPTE);

	return vPTE;
}

static int map_page(volatile struct pml4 *pPML4, void *virt, void *phys,
					unsigned int flags)
{
	volatile struct pdpt *pPDPT;
	volatile struct pd *pPD;
	volatile struct pt *pPT;
	volatile struct pte *vPTE;

	unsigned int df = F_WRITEABLE;

	if (phys == NULL)
		df = 0; // alloc page on fault

	if (select_pdpt(virt, df, pPML4, &pPDPT, true))
		return 1;

	if (select_pd(virt, df, pPDPT, &pPD, true))
		return 1;

	if (select_pt(virt, df, pPD, &pPT, true))
		return 1;

	select_page(virt, pPT, &vPTE);

	// update counts in parent structures
	pml4_mapped->count++;
	pdpt_mapped->count++;
	pd_mapped->count++;

	// map page
	if (phys) {
		vPTE->flags = F_PRESENT | flags;
		vPTE->address = (uint64_t)phys >> 12;
		vPTE->loaded = 1;
		invlpg(virt);
	} else {
		vPTE->flags = flags;
		vPTE->address = 0;
		vPTE->loaded = 0;
	}

	return 0;
}

static int map_pages(volatile struct pml4 *pPMl4, void *virt_start,
					 void *phys_start, unsigned int flags, long page_count)
{
	uint64_t pml4_o = -1, pdpt_o = -1, pd_o = -1;

	uint64_t pml4_n, pdpt_n, pd_n;

	volatile struct pdpt *pPDPT = NULL;
	volatile struct pd *pPD = NULL;
	volatile struct pt *pPT = NULL;
	volatile struct pte *vPTE = NULL;

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

		if (pPDPT == NULL || pml4_o != pml4_n) {
			if (select_pdpt(virt, df, pPMl4, &pPDPT, true))
				goto failed;
			pml4_o = pml4_n;
		}

		if (pPD == NULL || pdpt_o != pdpt_n) {
			if (select_pd(virt, df, pPDPT, &pPD, true))
				goto failed;
			pdpt_o = pdpt_n;
		}

		if (pPT == NULL || pd_o != pd_n) {
			if (select_pt(virt, df, pPD, &pPT, true))
				goto failed;
			pd_o = pd_n;
		}

		select_page(virt, pPT, &vPTE);

		// update counts in parent structures
		pml4_mapped->count++;
		pdpt_mapped->count++;
		pd_mapped->count++;

		// map page
		if (phys_start) {
			vPTE->flags = F_PRESENT | flags;
			vPTE->address = (uint64_t)phys >> 12;
			vPTE->loaded = 1;
		} else {
			vPTE->flags = flags;
			vPTE->address = 0;
			vPTE->loaded = 0;
		}

		if (flags & F_GLOBAL)
			invlpg(virt);
	}

	__asm__ volatile("mov %cr3, %rax; mov %rax, %cr3;");

	return 0;

failed:
	unmap_pages(pPMl4, virt, i);

	return 1;
}

void paging_init(void)
{
	// map pdpt
	kernel_pml4.entries[0].flags = F_PRESENT | F_WRITEABLE;
	kernel_pml4.entries[0].address = (uint64_t)(kernel_pdpt_0.entries) >> 12;

	// map pd0 & pd1
	kernel_pdpt_0.entries[0].flags = F_PRESENT | F_WRITEABLE;
	kernel_pdpt_0.entries[0].address = (uint64_t)(kernel_pd_0.entries) >> 12;
	kernel_pdpt_0.entries[1].flags = F_PRESENT | F_WRITEABLE;
	kernel_pdpt_0.entries[1].address = (uint64_t)(kernel_pd_1.entries) >> 12;

	// map pd0 entires (length N_IDENT_PTS)
	for (int i = 0; i < N_IDENT_PTS; i++) {
		kernel_pd_0.entries[i].flags = F_PRESENT | F_WRITEABLE;
		kernel_pd_0.entries[i].address = (uint64_t)(kernel_pd_0_ents[i].entries) >> 12;
		for (size_t j = 0; j < 512; j++) {
			kernel_pd_0_ents[i].entries[j].flags = F_PRESENT | F_WRITEABLE;
			kernel_pd_0_ents[i].entries[j].address = ((i*512+j) * PAGE_SIZE) >> 12;
		}
	}


	// map paging_pt
	kernel_pd_1.entries[0].flags = F_PRESENT | F_WRITEABLE;
	kernel_pd_1.entries[0].address = (uint64_t)(paging_pt.entries) >> 12;

	memsetv(paging_pt.entries, 0, 4096);

	// make sure we are using THESE pagetables
	// EFI doesnt on boot
	__asm__ volatile("mov %0, %%cr3" ::"r"(kernel_pml4.entries) : "memory");
}

volatile void *pml4_alloc(void)
{
	struct pml4 *pml4_phys = alloc_phys_page();
	struct pml4 *pml4 = kmapaddr(pml4_phys, NULL, PAGE_SIZE, F_WRITEABLE);
	memset(pml4, 0, PAGE_SIZE);

	if (map_pages(pml4_phys, kernel_start, kernel_start,
				  F_PRESENT | F_WRITEABLE,
				  (kernel_end - kernel_start) / PAGE_SIZE)) {
		free_phys_page(pml4_phys);
		kunmapaddr(pml4);
		return NULL;
	}

	kunmapaddr(pml4);
	return pml4_phys;
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
		virt = virtaddr_alloc(&ctx->virtctx, pages);
	if (virt == NULL)
		return NULL;

	if (map_pages((volatile struct pml4 *)ctx->pml4, virt, aligned_phys,
				  F_PRESENT | flags, pages)) {
		virtaddr_free(&ctx->virtctx, virt);
		return NULL;
	}

	return (char *)virt + error;
}

void mem_unmapaddr(mem_ctx_t ctx, void *virt)
{
	if (virt == NULL)
		return;

	long pages = virtaddr_free(&ctx->virtctx, virt);
	if (pages < 1)
		return;
	unmap_pages(&kernel_pml4, virt, pages);
}

void *mem_alloc_page(mem_ctx_t ctx, unsigned int flags, bool lazy)
{
	void *virt = virtaddr_alloc(&ctx->virtctx, 1);
	if (virt == NULL)
		return NULL;

	if (mem_alloc_page_at(ctx, virt, flags, lazy) == NULL) {
		virtaddr_free(&ctx->virtctx, virt);
		return NULL;
	}

	return virt;
}

void *mem_alloc_page_at(mem_ctx_t ctx, void *virt, unsigned int flags,
						bool lazy)
{
	void *phys = NULL;
	if (!lazy) {
		if ((phys = alloc_phys_page()) == NULL)
			return NULL;
	}

	if (map_page((volatile struct pml4 *)ctx->pml4, virt, phys, flags)) {
		if (phys)
			free_phys_page(phys);
		return NULL;
	}

	return virt;
}

void *mem_alloc_pages(mem_ctx_t ctx, size_t count, unsigned int flags,
					  bool lazy)
{
	void *virt = virtaddr_alloc(&ctx->virtctx, count);
	if (virt == NULL)
		return NULL;

	if (mem_alloc_pages_at(ctx, count, virt, flags, lazy) == NULL) {
		virtaddr_free(&ctx->virtctx, virt);
		return NULL;
	}

	return virt;
}

void *mem_alloc_pages_at(mem_ctx_t ctx, size_t count, void *virt,
						 unsigned int flags, bool lazy)
{
	void *phys = NULL;
	if (!lazy) {
		if ((phys = alloc_phys_pages(count)) == NULL)
			return NULL;
	}

	if (map_pages((volatile struct pml4 *)ctx->pml4, virt, phys, flags,
				  count)) {
		if (phys)
			free_phys_pages(phys, count);
		return NULL;
	}
	return virt;
}

void mem_free_pages(mem_ctx_t ctx, void *virt)
{
	if (virt == NULL)
		return;

	long pages = virtaddr_free(&ctx->virtctx, virt);
	if (pages == 1)
		unmap_page((volatile struct pml4 *)ctx->pml4, virt);
	else if (pages > 1)
		unmap_pages((volatile struct pml4 *)ctx->pml4, virt, pages);
}

int mem_load_page(mem_ctx_t ctx, void *virt_addr)
{
	volatile struct pte *page =
		get_page((volatile struct pml4 *)ctx->pml4, virt_addr);
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
