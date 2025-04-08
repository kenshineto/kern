/**
** @file	vmtables.h
**
** @author	CSCI-452 class of 20245
**
** @brief	Predefined VM tables
*/

#ifndef VMTABLES_H_
#define VMTABLES_H_

#include <defs.h>
#include <types.h>
#include <vm.h>

#ifndef ASM_SRC

/*
** Initial page directory, for when the kernel is starting up
**
** we use large (4MB) pages here to allow us to use a one-level
** paging hierarchy; the kernel will create a new page table
** hierarchy once memory is initialized
*/
extern pde_t firstpdir[];

/*
** "Identity" page map table.
**
** This just maps the first 4MB of physical memory. It is initialized
** in vm_init().
*/
extern pte_t id_map[];

/*
** Kernel address mappings, present in every page table
*/
extern mapping_t kmap[];
extern const uint32_t n_kmap;

#endif  /* !ASM_SRC */

#endif
