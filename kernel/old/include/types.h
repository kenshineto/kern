#ifndef TYPES_H_
#define TYPES_H_
#ifndef ASM_SRC

#ifdef KERNEL_SRC
// we define these here instead of in vm.h in order to get around a
// nasty chick/egg dependency between procs.h and vm.h
typedef uint32_t pde_t; // page directory entry
typedef uint32_t pte_t; // page table entry
#endif /* KERNEL_SRC */

#endif
#endif
