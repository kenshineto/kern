#include <lib.h>
#include <comus/memory.h>

#include "virtalloc.h"
#include "physalloc.h"
#include "paging.h"
#include "memory.h"
#include <stdint.h>

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
	uint64_t : 3; // ignored
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
			uint64_t : 1; // ignored
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

// kernel start/end
extern char kernel_start[];
extern char kernel_end[];

// invalidate page cache at a vitural address
static inline void invlpg(volatile const void *vADDR)
{
	__asm__ volatile("invlpg (%0)" ::"r"(vADDR) : "memory");
}

/* map */

// map a physical address to a virtural address
// @returns VIRTUAL ADDRESS
static volatile void *map_addr(volatile const void *pADDR, size_t pt_idx)
{
	volatile char *vADDR;
	volatile struct pte *vPTE;

	assert(pt_idx < 512, "invalid page table entry index");

	vADDR = (char *)(uintptr_t)(0x40000000 + pt_idx * PAGE_SIZE);
	vPTE = &paging_pt.entries[pt_idx];

	if ((uint64_t)pADDR >> 12 == vPTE->address)
		return vADDR;

	vPTE->address = (uint64_t)pADDR >> 12;
	vPTE->flags = F_PRESENT | F_WRITEABLE;
	invlpg(vADDR);
	return vADDR;
}

#define PML4_MAP(pADDR) (volatile struct pml4 *)map_addr(pADDR, 0)
#define PML4_MAPC(pADDR) (volatile const struct pml4 *)map_addr(pADDR, 4)

#define PDPT_MAP(pADDR) (volatile struct pdpt *)map_addr(pADDR, 1)
#define PDPT_MAPC(pADDR) (volatile const struct pdpt *)map_addr(pADDR, 5)

#define PD_MAP(pADDR) (volatile struct pd *)map_addr(pADDR, 2)
#define PD_MAPC(pADDR) (volatile const struct pd *)map_addr(pADDR, 6)

#define PT_MAP(pADDR) (volatile struct pt *)map_addr(pADDR, 3)
#define PT_MAPC(pADDR) (volatile const struct pt *)map_addr(pADDR, 7)

#define PAGE_MAP(pADDR) (volatile void *)map_addr(pADDR, 8)
#define PAGE_MAPC(pADDR) (volatile const void *)map_addr(pADDR, 9)

/* locate */

// locate a pdpt for a vitural address
// @returns PHYSICAL ADDRESS
static volatile struct pdpt *pdpt_locate(volatile struct pml4 *pPML4,
										 const void *vADDR)
{
	volatile struct pml4 *vPML4;
	volatile struct pml4e *vPML4E;
	volatile struct pdpt *pPDPT;
	uint64_t offset;

	offset = (uint64_t)vADDR >> 39;
	vPML4 = PML4_MAP(pPML4);
	vPML4E = &vPML4->entries[offset];

	if (vPML4E->flags & F_PRESENT) {
		pPDPT = (volatile struct pdpt *)((uintptr_t)vPML4E->address << 12);
		return pPDPT;
	}

	return NULL;
}

// locate a pd for a vitural address
// @returns PHYSICAL ADDRESS
static volatile struct pd *pd_locate(volatile struct pdpt *pPDPT,
									 const void *vADDR)
{
	volatile struct pdpt *vPDPT;
	volatile struct pdpte *vPDPTE;
	volatile struct pd *pPD;
	uint64_t offset;

	offset = ((uint64_t)vADDR >> 30) & 0x1ff;
	vPDPT = PDPT_MAP(pPDPT);
	vPDPTE = &vPDPT->entries[offset];

	if (vPDPTE->flags & F_PRESENT) {
		pPD = (volatile struct pd *)((uintptr_t)vPDPTE->address << 12);
		return pPD;
	}

	return NULL;
}

// locate a pt for a vitural address
// @returns PHYSICAL ADDRESS
static volatile struct pt *pt_locate(volatile struct pd *pPD, const void *vADDR)
{
	volatile struct pd *vPD;
	volatile struct pde *vPDE;
	volatile struct pt *pPT;
	uint64_t offset;

	offset = ((uint64_t)vADDR >> 21) & 0x1ff;
	vPD = PD_MAP(pPD);
	vPDE = &vPD->entries[offset];

	if (vPDE->flags & F_PRESENT) {
		pPT = (volatile struct pt *)((uintptr_t)vPDE->address << 12);
		return pPT;
	}

	return NULL;
}

/* alloc */

// allocate a pml4
// @returns PHYSICAL ADDRESS
static volatile struct pml4 *pml4_alloc(void)
{
	volatile struct pml4 *pPML4, *vPML4;

	pPML4 = alloc_phys_page();
	if (pPML4 == NULL)
		return NULL;

	vPML4 = PML4_MAP(pPML4);
	memsetv(vPML4, 0, sizeof(struct pml4));
	return pPML4;
}

// allocate a pdpt for a vitural address (if not exists)
// @returns PHYSICAL ADDRESS
static volatile struct pdpt *pdpt_alloc(volatile struct pml4 *pPML4,
										void *vADDR, unsigned int flags)
{
	volatile struct pml4 *vPML4;
	volatile struct pml4e *vPML4E;
	volatile struct pdpt *pPDPT, *vPDPT;
	uint64_t offset;

	offset = (uint64_t)vADDR >> 39;
	vPML4 = PML4_MAP(pPML4);
	vPML4E = &vPML4->entries[offset];

	pPDPT = pdpt_locate(pPML4, vADDR);
	if (pPDPT) {
		vPML4E->flags |= flags;
		return pPDPT;
	}

	pPDPT = alloc_phys_page();
	if (pPML4 == NULL)
		return NULL;

	vPDPT = PDPT_MAP(pPDPT);
	memsetv(vPDPT, 0, sizeof(struct pdpt));
	vPML4E->address = (uintptr_t)pPDPT >> 12;
	vPML4E->flags = F_PRESENT | flags;
	vPML4->count++;

	return pPDPT;
}

// allocate a pd for a vitural address (if not exists)
// @returns PHYSICAL ADDRESS
static volatile struct pd *pd_alloc(volatile struct pdpt *pPDPT, void *vADDR,
									unsigned int flags)
{
	volatile struct pdpt *vPDPT;
	volatile struct pdpte *vPDPTE;
	volatile struct pd *pPD, *vPD;
	uint64_t offset;

	offset = ((uint64_t)vADDR >> 30) & 0x1ff;
	vPDPT = PDPT_MAP(pPDPT);
	vPDPTE = &vPDPT->entries[offset];

	pPD = pd_locate(pPDPT, vADDR);
	if (pPD) {
		vPDPTE->flags |= flags;
		return pPD;
	}

	pPD = alloc_phys_page();
	if (pPDPT == NULL)
		return NULL;

	vPD = PD_MAP(pPD);
	memsetv(vPD, 0, sizeof(struct pd));
	vPDPTE->address = (uintptr_t)pPD >> 12;
	vPDPTE->flags = F_PRESENT | flags;
	vPDPT->count++;

	return pPD;
}

// allocate a pd for a vitural address (if not exists)
// @returns PHYSICAL ADDRESS
static volatile struct pt *pt_alloc(volatile struct pd *pPD, void *vADDR,
									unsigned int flags)
{
	volatile struct pd *vPD;
	volatile struct pde *vPDE;
	volatile struct pt *pPT, *vPT;
	uint64_t offset;

	offset = ((uint64_t)vADDR >> 21) & 0x1ff;
	vPD = PD_MAP(pPD);
	vPDE = &vPD->entries[offset];

	pPT = pt_locate(pPD, vADDR);
	if (pPT) {
		vPDE->flags |= flags;
		return pPT;
	}

	pPT = alloc_phys_page();
	if (pPD == NULL)
		return NULL;

	vPT = PT_MAP(pPT);
	memsetv(vPT, 0, sizeof(struct pt));
	vPDE->address = (uintptr_t)pPT >> 12;
	vPDE->flags = F_PRESENT | flags;
	vPD->count++;

	return pPT;
}

/* free */

static void pt_free(volatile struct pt *pPT, bool force)
{
	volatile struct pt *vPT;
	uint64_t count;

	vPT = PT_MAP(pPT);
	count = (vPT->count_high << 2) | vPT->count_low;

	if (!count)
		goto free;

	for (uint64_t i = 0; i < 512; i++) {
		volatile struct pte *vPTE;
		void *pADDR;

		vPTE = &vPT->entries[i];
		if (!(vPTE->flags & F_PRESENT))
			continue;

		pADDR = (void *)((uintptr_t)vPTE->address << 12);
		free_phys_page(pADDR);
		count--;
	}

	if (!force && count) {
		vPT->count_low = count;
		vPT->count_high = count >> 2;
		return;
	}

free:
	free_phys_page((void *)(uintptr_t)pPT);
}

static void pd_free(volatile struct pd *pPD, bool force)
{
	volatile struct pd *vPD;
	uint64_t count;

	vPD = PD_MAP(pPD);
	count = vPD->count;

	if (!count)
		goto free;

	for (uint64_t i = 0; i < 512; i++) {
		volatile struct pde *vPDE;
		volatile struct pt *pPT;

		vPDE = &vPD->entries[i];
		if (!(vPDE->flags & F_PRESENT))
			continue;

		pPT = (volatile struct pt *)((uintptr_t)vPDE->address << 12);
		pt_free(pPT, force);
		count--;
	}

	if (!force && count) {
		vPD->count = count;
		return;
	}

free:
	free_phys_page((void *)(uintptr_t)pPD);
}

static void pdpt_free(volatile struct pdpt *pPDPT, bool force)
{
	volatile struct pdpt *vPDPT;
	uint64_t count;

	vPDPT = PDPT_MAP(pPDPT);
	count = vPDPT->count;

	if (!count)
		goto free;

	for (uint64_t i = 0; i < 512; i++) {
		volatile struct pdpte *vPDPTE;
		volatile struct pd *pPD;

		vPDPTE = &vPDPT->entries[i];
		if (!(vPDPTE->flags & F_PRESENT))
			continue;

		pPD = (volatile struct pd *)((uintptr_t)vPDPTE->address << 12);
		pd_free(pPD, force);
		count--;
	}

	if (!force && count) {
		vPDPT->count = count;
		return;
	}

free:
	free_phys_page((void *)(uintptr_t)pPDPT);
}

static void pml4_free(volatile struct pml4 *pPML4, bool force)
{
	volatile struct pml4 *vPML4;
	uint64_t count;

	vPML4 = PML4_MAP(pPML4);
	count = vPML4->count;

	if (!count)
		goto free;

	for (uint64_t i = 0; i < 512; i++) {
		volatile struct pml4e *vPML4E;
		volatile struct pdpt *pPDPT;

		vPML4E = &vPML4->entries[i];
		if (!(vPML4E->flags & F_PRESENT))
			continue;

		pPDPT = (volatile struct pdpt *)((uintptr_t)vPML4E->address << 12);
		pdpt_free(pPDPT, force);
		count--;
	}

	if (!force && count) {
		vPML4->count = count;
		return;
	}

free:
	free_phys_page((void *)(uintptr_t)pPML4);
}

/* clone */

volatile void *page_clone(volatile void *old_pADDR, bool cow)
{
	volatile const void *old_vADDR;
	volatile void *new_pADDR, *new_vADDR;

	// TODO: cow
	(void) cow;

	// dont reallocate kernel memeory!!
	if ((volatile char *) old_pADDR <= kernel_end)
		return old_pADDR;

	new_pADDR = alloc_phys_page();
	if (new_pADDR == NULL)
		return NULL;

	old_vADDR = PAGE_MAPC(old_pADDR);
	new_vADDR = PAGE_MAP(new_pADDR);
	memcpyv(new_vADDR, old_vADDR, PAGE_SIZE);
	return new_pADDR;
}

volatile struct pt *pt_clone(volatile const struct pt *old_pPT,
								 bool cow)
{
	volatile const struct pt *old_vPT;
	volatile struct pt *new_pPT, *new_vPT;

	new_pPT = alloc_phys_page();
	if (new_pPT == NULL)
		return NULL;

	old_vPT = PT_MAPC(old_pPT);
	new_vPT = PT_MAP(new_pPT);
	memsetv(new_vPT, 0, PAGE_SIZE);

	new_vPT->count_high = old_vPT->count_high;
	new_vPT->count_low = old_vPT->count_low;

	for (size_t i = 0; i < 512; i++) {
		volatile const struct pte *old_vPTE;
		volatile struct pte *new_vPTE;
		volatile void *old_pADDR, *new_pADDR;

		old_vPTE = &old_vPT->entries[i];
		new_vPTE = &new_vPT->entries[i];

		new_vPTE->execute_disable = old_vPTE->execute_disable;
		new_vPTE->flags = old_vPTE->flags;
		if (!(old_vPTE->flags & F_PRESENT))
			continue;

		new_vPTE->execute_disable = old_vPTE->execute_disable;
		new_vPTE->flags = old_vPTE->flags;

		old_pADDR =
			(volatile void *)((uintptr_t)old_vPTE->address
										   << 12);
		new_pADDR = page_clone(old_pADDR, cow);
		if (new_pADDR == NULL)
			goto fail;

		new_vPTE->address = (uint64_t)new_pADDR >> 12;
	}

	return new_pPT;

fail:
	pt_free(new_pPT, true);
	return NULL;
}

volatile struct pd *pd_clone(volatile const struct pd *old_pPD,
								 bool cow)
{
	volatile const struct pd *old_vPD;
	volatile struct pd *new_pPD, *new_vPD;

	new_pPD = alloc_phys_page();
	if (new_pPD == NULL)
		return NULL;

	old_vPD = PD_MAPC(old_pPD);
	new_vPD = PD_MAP(new_pPD);
	memsetv(new_vPD, 0, PAGE_SIZE);

	new_vPD->count = old_vPD->count;

	for (size_t i = 0; i < 512; i++) {
		volatile const struct pde *old_vPDE;
		volatile struct pde *new_vPDE;
		volatile const struct pt *old_pPT;
		volatile struct pt *new_pPT;

		old_vPDE = &old_vPD->entries[i];
		new_vPDE = &new_vPD->entries[i];

		new_vPDE->execute_disable = old_vPDE->execute_disable;
		new_vPDE->flags = old_vPDE->flags;
		if (!(old_vPDE->flags & F_PRESENT))
			continue;

		old_pPT =
			(volatile const struct pt *)((uintptr_t)old_vPDE->address
										   << 12);
		new_pPT = pt_clone(old_pPT, cow);
		if (new_pPT == NULL)
			goto fail;

		new_vPDE->address = (uint64_t)new_pPT >> 12;
	}

	return new_pPD;

fail:
	pd_free(new_pPD, true);
	return NULL;
}

volatile struct pdpt *pdpt_clone(volatile const struct pdpt *old_pPDPT,
								 bool cow)
{
	volatile const struct pdpt *old_vPDPT;
	volatile struct pdpt *new_pPDPT, *new_vPDPT;

	new_pPDPT = alloc_phys_page();
	if (new_pPDPT == NULL)
		return NULL;

	old_vPDPT = PDPT_MAPC(old_pPDPT);
	new_vPDPT = PDPT_MAP(new_pPDPT);
	memsetv(new_vPDPT, 0, PAGE_SIZE);

	new_vPDPT->count = old_vPDPT->count;

	for (size_t i = 0; i < 512; i++) {
		volatile const struct pdpte *old_vPDPTE;
		volatile struct pdpte *new_vPDPTE;
		volatile const struct pd *old_pPD;
		volatile struct pd *new_pPD;

		old_vPDPTE = &old_vPDPT->entries[i];
		new_vPDPTE = &new_vPDPT->entries[i];

		new_vPDPTE->execute_disable = old_vPDPTE->execute_disable;
		new_vPDPTE->flags = old_vPDPTE->flags;
		if (!(old_vPDPTE->flags & F_PRESENT))
			continue;

		old_pPD =
			(volatile const struct pd *)((uintptr_t)old_vPDPTE->address
										   << 12);
		new_pPD = pd_clone(old_pPD, cow);
		if (new_pPD == NULL)
			goto fail;

		new_vPDPTE->address = (uint64_t)new_pPD >> 12;
	}

	return new_pPDPT;

fail:
	pdpt_free(new_pPDPT, true);
	return NULL;
}

volatile struct pml4 *pml4_clone(volatile const struct pml4 *old_pPML4,
								 bool cow)
{
	volatile const struct pml4 *old_vPML4;
	volatile struct pml4 *new_pPML4, *new_vPML4;

	new_pPML4 = pml4_alloc();
	if (new_pPML4 == NULL)
		return NULL;

	old_vPML4 = PML4_MAPC(old_pPML4);
	new_vPML4 = PML4_MAP(new_pPML4);

	new_vPML4->count = old_vPML4->count;

	for (size_t i = 0; i < 512; i++) {
		volatile const struct pml4e *old_vPML4E;
		volatile struct pml4e *new_vPML4E;
		volatile const struct pdpt *old_pPDPT;
		volatile struct pdpt *new_pPDPT;

		old_vPML4E = &old_vPML4->entries[i];
		new_vPML4E = &new_vPML4->entries[i];

		new_vPML4E->execute_disable = old_vPML4E->execute_disable;
		new_vPML4E->flags = old_vPML4E->flags;
		if (!(old_vPML4E->flags & F_PRESENT))
			continue;

		old_pPDPT =
			(volatile const struct pdpt *)((uintptr_t)old_vPML4E->address
										   << 12);
		new_pPDPT = pdpt_clone(old_pPDPT, cow);
		if (new_pPDPT == NULL)
			goto fail;

		new_vPML4E->address = (uint64_t)new_pPDPT >> 12;
	}

	return new_pPML4;

fail:
	pml4_free(new_pPML4, true);
	return NULL;
}

/* page specific */

// locate a pte for a vitural address
// @returns VIRTUAL ADDRESS
static volatile struct pte *page_locate(volatile struct pml4 *pPML4,
										const void *vADDR)
{
	volatile struct pdpt *pPDPT;
	volatile struct pd *pPD;
	volatile struct pt *pPT, *vPT;
	volatile struct pte *vPTE;
	uint64_t offset;

	pPDPT = pdpt_locate(pPML4, vADDR);
	if (pPDPT == NULL)
		return NULL;

	pPD = pd_locate(pPDPT, vADDR);
	if (pPD == NULL)
		return NULL;

	pPT = pt_locate(pPD, vADDR);
	if (pPT == NULL)
		return NULL;

	offset = ((uint64_t)vADDR >> 12) & 0x1ff;
	vPT = PT_MAP(pPT);
	vPTE = &vPT->entries[offset];

	if (vPTE->flags & F_PRESENT)
		return vPTE;

	return NULL;
}

// allocate a pte for a vitural address
// @returns VIRTUAL ADDRESS
static volatile struct pte *page_alloc(volatile struct pml4 *pPML4, void *vADDR,
									   unsigned int flags)
{
	volatile struct pdpt *pPDPT;
	volatile struct pd *pPD;
	volatile struct pt *pPT, *vPT;
	volatile struct pte *vPTE;
	uint64_t offset, count;

	pPDPT = pdpt_alloc(pPML4, vADDR, flags);
	if (pPDPT == NULL)
		return NULL;

	pPD = pd_alloc(pPDPT, vADDR, flags);
	if (pPD == NULL)
		return NULL;

	pPT = pt_alloc(pPD, vADDR, flags);
	if (pPT == NULL)
		return NULL;

	offset = ((uint64_t)vADDR >> 12) & 0x1ff;
	vPT = PT_MAP(pPT);
	vPTE = &vPT->entries[offset];

	memsetv(vPTE, 0, sizeof(struct pte));
	count = (vPT->count_high << 2) | vPT->count_low;
	count++;
	vPT->count_low = count & 0x3;
	vPT->count_high = (count >> 2) & 0x7f;

	return vPTE;
}

// free a pte (page) for a vitural address
static void page_free(volatile struct pml4 *pPML4, const void *vADDR,
					  bool deallocate)
{
	volatile struct pte *vPTE;
	void *pADDR;

	vPTE = page_locate(pPML4, vADDR);
	if (vPTE == NULL)
		return;

	vPTE->flags = 0;
	vPTE->address = 0;

	if (deallocate) {
		pADDR = (void *)((uintptr_t)vPTE->address << 12);
		free_phys_page(pADDR);
	}
}

/* map & unmap pages */

static void unmap_pages(volatile struct pml4 *pPML4, const void *vADDR,
						long page_count, bool deallocate)
{
	for (long i = 0; i < page_count; i++) {
		page_free(pPML4, vADDR, deallocate);
		vADDR = (char *)vADDR + PAGE_SIZE;
	}
}

static int map_pages(volatile struct pml4 *pPML4, void *vADDR, void *pADDR,
					 unsigned int flags, long page_count)
{
	volatile struct pte *vPTE;
	for (long i = 0; i < page_count; i++) {
		vPTE = page_alloc(pPML4, vADDR, flags);
		if (vPTE == NULL)
			goto fail;
		vPTE->address = (uint64_t)pADDR >> 12;
		vPTE->flags = F_PRESENT | flags;

		pADDR = (char *)pADDR + PAGE_SIZE;
		vADDR = (char *)vADDR + PAGE_SIZE;
	}
	return 0;

fail:
	unmap_pages(pPML4, vADDR, page_count, true);
	return 1;
}

/* other fns */

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
		kernel_pd_0.entries[i].address =
			(uint64_t)(kernel_pd_0_ents[i].entries) >> 12;
		for (size_t j = 0; j < 512; j++) {
			kernel_pd_0_ents[i].entries[j].flags = F_PRESENT | F_WRITEABLE;
			kernel_pd_0_ents[i].entries[j].address =
				((i * 512 + j) * PAGE_SIZE) >> 12;
		}
	}

	// map paging_pt
	kernel_pd_1.entries[0].flags = F_PRESENT | F_WRITEABLE;
	kernel_pd_1.entries[0].address = (uint64_t)(paging_pt.entries) >> 12;

	memsetv(paging_pt.entries, 0, PAGE_SIZE);

	// make sure we are using THESE pagetables
	// EFI doesnt on boot
	__asm__ volatile("mov %0, %%cr3" ::"r"(kernel_pml4.entries) : "memory");
}

volatile void *pgdir_alloc(void)
{
	volatile struct pml4 *pPML4;

	pPML4 = pml4_alloc();
	if (pPML4 == NULL)
		return NULL;

	if (map_pages(pPML4, kernel_start, kernel_start, F_PRESENT | F_WRITEABLE,
				  (kernel_end - kernel_start) / PAGE_SIZE)) {
		pml4_free(pPML4, false);
		return NULL;
	}

	return pPML4;
}

volatile void *pgdir_clone(volatile const void *old_pgdir, bool cow)
{
	return pml4_clone((volatile const struct pml4 *)old_pgdir, cow);
}

void pgdir_free(volatile void *addr)
{
	pml4_free(addr, true);
}

void *mem_mapaddr(mem_ctx_t ctx, void *phys, void *virt, size_t len,
				  unsigned int flags)
{
	long pages;
	size_t error;
	void *aligned_phys;

	error = (size_t)phys % PAGE_SIZE;
	len += error;
	pages = (len + PAGE_SIZE - 1) / PAGE_SIZE;
	aligned_phys = (char *)phys - error;

	// get page aligned (or allocate) vitural address
	if (virt == NULL)
		virt = virtaddr_alloc(&ctx->virtctx, pages);
	if (virt == NULL)
		return NULL;

	if (virtaddr_take(&ctx->virtctx, virt, pages))
		return NULL;

	assert((uint64_t)virt % PAGE_SIZE == 0,
		   "mem_mapaddr: vitural address not page aligned");

	if (map_pages((volatile struct pml4 *)ctx->pml4, virt, aligned_phys,
				  F_PRESENT | flags, pages)) {
		virtaddr_free(&ctx->virtctx, virt);
		return NULL;
	}

	return (char *)virt + error;
}

void *kmapuseraddr(mem_ctx_t ctx, const void *usrADDR, size_t len)
{
	volatile struct pml4 *pml4;
	char *pADDR, *vADDR;
	size_t npages, error, i;

	pml4 = (volatile struct pml4 *)kernel_mem_ctx->pml4;
	npages = (len + PAGE_SIZE - 1) / PAGE_SIZE;
	error = (size_t)usrADDR % PAGE_SIZE;
	vADDR = virtaddr_alloc(&kernel_mem_ctx->virtctx, npages);
	if (vADDR == NULL)
		return NULL;

	if (virtaddr_take(&kernel_mem_ctx->virtctx, vADDR, npages))
		return NULL;

	assert((size_t)vADDR % PAGE_SIZE == 0,
		   "kmapuseraddr: vitural address not page aligned");

	for (i = 0; i < npages; i++) {
		pADDR = mem_get_phys(ctx, (char *)usrADDR + i * PAGE_SIZE);
		if (pADDR == NULL)
			goto fail;

		// page align
		pADDR = (char *)(((size_t)pADDR / PAGE_SIZE) * PAGE_SIZE);

		if (map_pages(pml4, vADDR + i * PAGE_SIZE, pADDR,
					  F_PRESENT | F_WRITEABLE, 1))
			goto fail;
	}

	return vADDR + error;

fail:
	unmap_pages(&kernel_pml4, vADDR, i, false);
	virtaddr_free(&kernel_mem_ctx->virtctx, vADDR);
	return NULL;
}

void mem_unmapaddr(mem_ctx_t ctx, const void *virt)
{
	long pages;

	if (virt == NULL)
		return;

	// page align
	virt = (void *)(((size_t)virt / PAGE_SIZE) * PAGE_SIZE);

	pages = virtaddr_free(&ctx->virtctx, virt);
	if (pages < 1)
		return;
	unmap_pages(&kernel_pml4, virt, pages, false);
}

void *mem_get_phys(mem_ctx_t ctx, const void *vADDR)
{
	char *pADDR;
	volatile struct pte *vPTE;

	vPTE = page_locate((volatile struct pml4 *)ctx->pml4, vADDR);
	if (vPTE == NULL)
		return NULL;

	pADDR = (void *)((uintptr_t)vPTE->address << 12);
	pADDR += ((uint64_t)vADDR % PAGE_SIZE);
	return pADDR;
}

void *mem_alloc_page(mem_ctx_t ctx, unsigned int flags)
{
	return mem_alloc_pages(ctx, 1, flags);
}

void *mem_alloc_page_at(mem_ctx_t ctx, void *virt, unsigned int flags)
{
	return mem_alloc_pages_at(ctx, 1, virt, flags);
}

void *mem_alloc_pages(mem_ctx_t ctx, size_t count, unsigned int flags)
{
	void *virt = virtaddr_alloc(&ctx->virtctx, count);
	if (virt == NULL)
		return NULL;

	if (mem_alloc_pages_at(ctx, count, virt, flags) == NULL) {
		virtaddr_free(&ctx->virtctx, virt);
		return NULL;
	}

	return virt;
}

void *mem_alloc_pages_at(mem_ctx_t ctx, size_t count, void *virt,
						 unsigned int flags)
{
	size_t pages_needed = count;

	struct phys_page_slice prev_phys_block = PHYS_PAGE_SLICE_NULL;
	struct phys_page_slice phys_pages;

	if (virtaddr_take(&ctx->virtctx, virt, count))
		return NULL;

	while (pages_needed > 0) {
		phys_pages = alloc_phys_page_withextra(pages_needed);
		if (phys_pages.pagestart == NULL) {
			goto mem_alloc_pages_at_fail;
		}

		{
			// allocate the first page and store in it the physical address of the
			// previous chunk of pages
			// TODO: skip this if there are already enough pages from first alloc
			void *pageone = kmapaddr(phys_pages.pagestart, NULL, 1,
									 F_PRESENT | F_WRITEABLE);
			if (pageone == NULL) {
				panic("kernel out of virtual memory");
			}
			*((struct phys_page_slice *)pageone) = prev_phys_block;
			prev_phys_block = phys_pages;
			kunmapaddr(pageone);
		}

		// index into virtual page array at index [count - pages_needed]
		void *vaddr = ((uint8_t *)virt) + ((count - pages_needed) * PAGE_SIZE);

		assert(pages_needed >= phys_pages.num_pages, "overflow");
		pages_needed -= phys_pages.num_pages;

		if (map_pages((volatile struct pml4 *)ctx->pml4, vaddr,
					  phys_pages.pagestart, flags, phys_pages.num_pages)) {
			goto mem_alloc_pages_at_fail;
		}
	}

	return virt;

mem_alloc_pages_at_fail:
	while (prev_phys_block.pagestart) {
		void *virtpage = kmapaddr(prev_phys_block.pagestart, NULL, 1,
								  F_PRESENT | F_WRITEABLE);
		if (!virtpage) {
			// memory corruption, most likely a bug
			// could also ERROR here and exit with leak
			panic("unable to free memory from failed mem_alloc_pages_at call");
		}
		struct phys_page_slice prev = *(struct phys_page_slice *)virtpage;
		prev_phys_block = prev;
		free_phys_pages_slice(prev);
		kunmapaddr(virtpage);
	}

	return NULL;
}

void mem_free_pages(mem_ctx_t ctx, const void *virt)
{
	if (virt == NULL)
		return;

	long pages = virtaddr_free(&ctx->virtctx, virt);
	unmap_pages((volatile struct pml4 *)ctx->pml4, virt, pages, true);
}
