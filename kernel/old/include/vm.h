/**
** @file	vm.h
**
** @author	CSCI-452 class of 20245
**
** @brief	Virtual memory-related declarations.
*/

#ifndef VM_H_
#define VM_H_

#include <defs.h>
#include <types.h>

#include <procs.h>

/*
** VM layout of the system
**
** User processes use the first 4MB of the 32-bit address space; see the
** next comment for details.
**
** Kernel virtual addresses are in the "higher half" range, beginning
** at 0x80000000.  We define our mapping such that virtual address
** 0x8nnnnnnn maps to physical address 0x0nnnnnnn, so converting between
** the two is trivial.
*/

/*
** VM layout of process' address space
**
** Processes are limited to the first 4MB of the 32-bit address space:
**
**  Address Range            Contents
**  =======================  ================================
**  0x00000000 - 0x00000fff  page 0 is inaccessible
**  0x00001000 - 0x000..fff  text occupies pages 1 - N
**  0x000..000 - 0x000..fff  data occupies pages N+1 - N+d
**  0x000..000 - 0x000..fff  bss occupies pages N+d+1 - N+d+b
**  0x000..000 - 0x003fdfff  unusable
**  0x003fe000 - 0x003fffff  stack occupies last two pages
**
** This gives us the following page table structure:
**
** Page directory:
**   Entries    Contents
**   ========   ==============================
**   0          point to PMT for address space
**   1 - 1023   invalid
**
** Page map table:
**   Entries         Contents
**   ========        ==============================
**   0               invalid
**   1 - N           text frames
**   N+1 - N+d       data frames
**   N+d+1 - N+d+b   bss frames
**   N+d+b+1 - 1021  invalid
**   1022 - 1023     stack frames
*/

/*
** General (C and/or assembly) definitions
*/

// user virtual addresses
#define USER_TEXT      0x00001000
#define USER_STACK     0x003fe000
#define USER_STK_END   0x00400000

// how to find the addresses of the stack pages in the VM hierarchy
// user address space is the first 4MB of virtual memory
#define USER_PDE       0
// the stack occupies the last two pages of the address space
#define USER_STK_PTE1  1022
#define USER_STK_PTE2  1023

// some important memory addresses
#define KERN_BASE      0x80000000    // start of "kernel" memory
#define EXT_BASE       0x00100000    // start of "extended" memory (1MB)
#define DEV_BASE       0xfe000000    // "device" memory
#define PHYS_TOP       0x3fffffff    // last usable physical address (1GB - 1)

// where the kernel actually lives
#define KERN_PLINK     0x00010000
#define KERN_VLINK     (KERN_BASE + KERN_PLINK)

// number of entries in a page directory or page table
#define N_PDE          1024
#define N_PTE          1024

// index field shift counts and masks
#define PDIX_SHIFT     22
#define PTIX_SHIFT     12
#define PIX2I_MASK     0x3ff

// physical/virtual converters that don't use casting
// (usable from anywhere)
#define V2PNC(a)       ((a) - KERN_BASE)
#define P2VNC(a)       ((a) + KERN_BASE)

// page-size address rounding macros
#define SZ_PG_M1       MOD4K_BITS
#define SZ_PG_MASK     MOD4K_MASK
#define PGUP(a)        (((a)+SZ_PG_M1) & SZ_PG_MASK)
#define PGDOWN(a)      ((a) & SZ_PG_MASK)

// page directory entry bit fields
#define PDE_P          0x00000001	// 1 = present
#define PDE_RW         0x00000002	// 1 = writable
#define PDE_US         0x00000004	// 1 = user and system usable
#define PDE_PWT        0x00000008	// cache: 1 = write-through
#define PDE_PCD        0x00000010	// cache: 1 = disabled
#define PDE_A          0x00000020	// accessed
#define PDE_D          0x00000040	// dirty (4MB pages)
#define PDE_AVL1       0x00000040	// ignored (4KB pages)
#define PDE_PS         0x00000080	// 1 = 4MB page size
#define PDE_G          0x00000100	// global
#define PDE_AVL2       0x00000e00	// ignored
#define PDE_PAT        0x00001000	// (4MB pages) use page attribute table
#define PDE_PTA        0xfffff000	// page table address field (4KB pages)
#define PDE_FA         0xffc00000	// frame address field (4MB pages)

// page table entry bit fields
#define PTE_P          0x00000001	// present
#define PTE_RW         0x00000002	// 1 = writable
#define PTE_US         0x00000004	// 1 = user and system usable
#define PTE_PWT        0x00000008	// cache: 1 = write-through
#define PTE_PCD        0x00000010	// cache: 1 = disabled
#define PTE_A          0x00000020	// accessed
#define PTE_D          0x00000040	// dirty
#define PTE_PAT        0x00000080	// use page attribute table
#define PTE_G          0x00000100	// global
#define PTE_AVL2       0x00000e00	// ignored
#define PTE_FA         0xfffff000	// frame address field

// error code bit assignments for page faults
#define PFLT_P         0x00000001
#define PFLT_W         0x00000002
#define PFLT_US        0x00000004
#define PFLT_RSVD      0x00000008
#define PFLT_ID        0x00000010
#define PFLT_PK        0x00000020
#define PFLT_SS        0x00000040
#define PFLT_HLAT      0x00000080
#define PFLT_SGX       0x00008000
#define PFLT_UNUSED    0xffff7f00

#ifndef ASM_SRC

/*
** Start of C-only definitions
*/

// physical/virtual converters that do use casting
// (not usable from assembly)
#define V2P(a)         (((uint32_t)(a)) - KERN_BASE)
#define P2V(a)         (((uint32_t)(a)) + KERN_BASE)

// create a pde/pte from an integer frame number and permission bits
#define MKPDE(f,p)  ((pde_t)( TO_FRAME((f)) | (p) ))
#define MKPTE(f,p)  ((pte_t)( TO_FRAME((f)) | (p) ))

// is a PDE/PTE present?
// (P bit is in the same place in both)
#define IS_PRESENT(entry)  (((entry) & PDE_P) != 0 )

// is a PDE a 4MB page entry?
#define IS_LARGE(pde)      (((pde) & PDE_PS) != 0 )

// is this entry "system only" or "system and user"?
#define IS_SYSTEM(entry)   (((entry) & PDE_US) == 0 )
#define IS_USER(entry)     (((entry) & PDE_US) != 0 )

// low-order nine bits of PDEs and PTEs hold "permission" flag bits
#define PERMS_MASK          MOD4K_MASK

// 4KB frame numbers are 20 bits wide
#define	FRAME_4K_SHIFT 12
#define FRAME2I_4K_MASK    0x000fffff
#define TO_4KFRAME(n)      (((n)&FRAME2I_4K_MASK) << FRAME_4K_SHIFT)
#define GET_4KFRAME(n)     (((n) >> FRAME_4K_SHIFT)&F2I_4K_MASK)
#define PDE_4K_ADDR(n)     ((n) & MOD4K_MASK)
#define PTE_4K_ADDR(n)     ((n) & MOD4K_MASK)

// 4MB frame numbers are 10 bits wide
#define	FRAME_4M_SHIFT 22
#define FRAME2I_4M_MASK    0x000003ff
#define TO_4MFRAME(n)      (((n)&FRAME2I_4M_MASK) << FRAME_4M_SHIFT)
#define GET_4MFRAME(n)     (((n) >> FRAME_4M_SHIFT)&F2I_4M_MASK)
#define PDE_4M_ADDR(n)     ((n) & MOD4M_MASK)
#define PTE_4M_ADDR(n)     ((n) & MOD4M_MASK)

// extract the PMT address or frame address from a table entry
// PDEs could point to 4MB pages, or 4KB PMTs
#define PDE_ADDR(p)    (IS_LARGE(p)?(((uint32_t)p)&PDE_FA):(((uint32_t)p)&PDE_PTA))
// PTEs always point to 4KB pages
#define PTE_ADDR(p)    (((uint32_t)(p))&PTE_FA)
// everything has nine bits of permission flags
#define PERMS(p)       (((uint32_t)(p))&PERMS_MASK)

// extract the table indices from a 32-bit address
#define PDIX(v)        ((((uint32_t)(v)) >> PDIX_SHIFT) & PIX2I_MASK)
#define PTIX(v)        ((((uint32_t)(v)) >> PTIX_SHIFT) & PIX2I_MASK)

/*
** Types
*/

// page directory entries

// as a 32-bit word, in types.h
// typedef uint32_t pde_t;

// PDE for 4KB pages
typedef struct pdek_s {
	uint_t p    :1;   // present
	uint_t rw   :1;   // writable
	uint_t us   :1;   // user/supervisor
	uint_t pwt  :1;   // cache write-through
	uint_t pcd  :1;   // cache disable
	uint_t a    :1;   // accessed
	uint_t avl1 :1;   // ignored (available)
	uint_t ps   :1;   // page size (must be 0)
	uint_t avl2 :4;   // ignored (available)
	uint_t fa   :20;  // frame address
} pdek_f_t;

// PDE for 4MB pages
typedef struct pdem_s {
	uint_t p    :1;   // present
	uint_t rw   :1;   // writable
	uint_t us   :1;   // user/supervisor
	uint_t pwt  :1;   // cache write-through
	uint_t pcd  :1;   // cache disable
	uint_t a    :1;   // accessed
	uint_t d    :1;   // dirty
	uint_t ps   :1;   // page size (must be 1)
	uint_t g    :1;   // global
	uint_t avl  :3;   // ignored (available)
	uint_t fa   :20;  // frame address
} pdem_f_t;

// page table entries

// as a 32-bit word, in types.h
// typedef uint32_t pte_t;

// broken out into fields
typedef struct pte_s {
	uint_t p   :1;    // present
	uint_t rw  :1;    // writable
	uint_t us  :1;    // user/supervisor
	uint_t pwt :1;    // cache write-through
	uint_t pcd :1;    // cache disable
	uint_t a   :1;    // accessed
	uint_t d   :1;    // dirty
	uint_t pat :1;    // page attribute table in use
	uint_t g   :1;    // global
	uint_t avl :3;    // ignored (available)
	uint_t fa  :20;   // frame address
} ptef_t;

// page fault error code bits
// comment: meaning when 1 / meaning when 0
struct pfec_s {
	uint_t p    :1;		// page-level protection violation / !present
	uint_t w    :1;		// write / read
	uint_t us   :1;		// user-mode access / supervisor-mode access
	uint_t rsvd :1;		// reserved bit violation / not
	uint_t id   :1;		// instruction fetch / data fetch
	uint_t pk   :1;		// protection-key violation / !pk
	uint_t ss   :1;		// shadow stack access / !ss
	uint_t hlat :1;		// HLAT paging / ordinary paging or access rights
	uint_t xtr1 :7;		// unused
	uint_t sgz  :1;		// SGX-specific access control violation / !SGX
	uint_t xtr2 :16;	// more unused
};

typedef union pfec_u {
	uint32_t u;
	struct pfec_s s;
} pfec_t;

// Mapping descriptor for VA::PA mappings
typedef struct mapping_t {
	uint32_t va_start;  // starting virtual address for this range
	uint32_t pa_start;  // first physical address in the range
	uint32_t pa_end;    // last physical address in the range
	uint32_t perm;      // access control
} mapping_t;

/*
** Globals
*/

// created page directory for the kernel
extern pde_t *kpdir;

/*
** Prototypes
*/

/**
** Name:	vm_init
**
** Initialize the VM module
**
** Note: should not be called until after the memory free list has
** been set up.
*/
void vm_init( void );

/**
** Name:    vm_pagedup
**
** Duplicate a page of memory
**
** @param old  Pointer to the first byte of a page
**
** @return a pointer to the new, duplicate page, or NULL
*/
void *vm_pagedup( void *old );

/**
** Name:    vm_ptdup
**
** Duplicate a page directory entry
**
** @param dst   Pointer to where the duplicate should go
** @param curr  Pointer to the entry to be duplicated
**
** @return true on success, else false
*/
bool_t vm_ptdup( pde_t *dst, pde_t *curr );

/**
** Name:	vm_getpte
**
** Return the address of the PTE corresponding to the virtual address
** 'va' within the address space controlled by 'pgdir'. If there is no
** page table for that VA and 'alloc' is true, create the necessary
** page table entries.
**
** @param pdir   Pointer to the page directory to be searched
** @param va     The virtual address we're looking for
** @param alloc  Should we allocate a page table if there isn't one?
**
** @return A pointer to the page table entry for this VA, or NULL
*/
pte_t *vm_getpte( pde_t *pdir, const void *va, bool_t alloc );

/**
** Name:	vm_mkkvm
**
** Create the kernel's page table hierarchy
*/
pde_t *vm_mkkvm( void );

/**
** Name:	vm_mkuvm
**
** Create the page table hierarchy for a user process
*/
pde_t *vm_mkuvm( void );

/**
** Name:	vm_set_kvm
**
** Switch the page table register to the kernel's page directory
*/
void vm_set_kvm( void );

/**
** Name:	vm_set_uvm
**
** Switch the page table register to the page directory for a user process.
**
** @param p   The PCB of the user process
*/
void vm_set_uvm( pcb_t *p );

/**
** Name:    vm_add
**
** Add pages to the page hierarchy for a process, copying data into
** them if necessary.
**
** @param pdir   Pointer to the page directory to modify
** @param wr     "Writable" flag for the PTE
** @param sys    "System" flag for the PTE
** @param va     Starting VA of the range
** @param size   Amount of physical memory to allocate
** @param data   Pointer to data to copy, or NULL
** @param bytes  Number of bytes to copy
**
** @return status of the allocation attempt
*/
int vm_add( pde_t *pdir, bool_t wr, bool_t sys,
		void *va, uint32_t size, char *data, uint32_t bytes );

/**
** Name:	vm_free
**
** Deallocate a page table hierarchy and all physical memory frames
** in the user portion.
**
** @param pdir  Pointer to the page directory
*/
void vm_free( pde_t *pdir );

/*
** Name:	vm_map
**
** Create PTEs for virtual addresses starting at 'va' that refer to
** physical addresses in the range [pa, pa+size-1]. We aren't guaranteed
** that va is page-aligned.
**
** @param pdir  Page directory for this address space
** @param va    The starting virtual address
** @param size  Length of the range to be mapped
** @param pa    The starting physical address
** @param perm  Permission bits for the PTEs
*/
int vm_map( pde_t *pdir, void *va, uint_t size, uint_t pa, int perm );

/**
** Name:    vm_uvmdup
**
** Create a duplicate of the user portio of an existing page table
** hierarchy. We assume that the "new" page directory exists and
** the system portions of it should not be touched.
**
** @param old  Existing page directory
** @param new  New page directory
**
** @return status of the duplication attempt
*/
int vm_uvmdup( pde_t *old, pde_t *new );

#endif  /* !ASM_SRC */

#endif
