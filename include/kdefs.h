/**
** @file	kdefs.h
**
** @author	CSCI-452 class of 20245
**
** @brief	Kernel-only declarations.
*/

#ifndef KDEFS_H_
#define KDEFS_H_

// debugging macros
#include <debug.h>

/*
** General (C and/or assembly) definitions
*/

// page sizes
#define SZ_PAGE NUM_4KB
#define SZ_BIGPAGE NUM_4MB

// kernel stack size (bytes)
#define N_KSTKPAGES 1
#define SZ_KSTACK (N_KSTKPAGES * SZ_PAGE)

// user stack size
#define N_USTKPAGES 2
#define SZ_USTACK (N_USTKPAGES * SZ_PAGE)

// declarations for modulus checking of (e.g.) sizes and addresses

#define MOD4_BITS 0x00000003
#define MOD4_MASK 0xfffffffc
#define MOD4_INC 0x00000004
#define MOD4_SHIFT 2

#define MOD16_BITS 0x0000000f
#define MOD16_MASK 0xfffffff0
#define MOD16_INC 0x00000010
#define MOD16_SHIFT 4

#define MOD1K_BITS 0x000003ff
#define MOD1K_MASK 0xfffffc00
#define MOD1K_INC 0x00000400
#define MOD1K_SHIFT 10

#define MOD4K_BITS 0x00000fff
#define MOD4K_MASK 0xfffff000
#define MOD4K_INC 0x00001000
#define MOD4K_SHIFT 12

#define MOD1M_BITS 0x000fffff
#define MOD1M_MASK 0xfff00000
#define MOD1M_INC 0x00100000
#define MOD1M_SHIFT 20

#define MOD4M_BITS 0x003fffff
#define MOD4M_MASK 0xffc00000
#define MOD4M_INC 0x00400000
#define MOD4M_SHIFT 22

#define MOD1G_BITS 0x3fffffff
#define MOD1G_MASK 0xc0000000
#define MOD1G_INC 0x40000000
#define MOD1G_SHIFT 30

#ifndef ASM_SRC

/*
** Start of C-only definitions
*/

// unit conversion macros
#define B_TO_KB(x) (((uint_t)(x)) >> 10)
#define B_TO_MB(x) (((uint_t)(x)) >> 20)
#define B_TO_GB(x) (((uint_t)(x)) >> 30)

#define KB_TO_B(x) (((uint_t)(x)) << 10)
#define KB_TO_MB(x) (((uint_t)(x)) >> 10)
#define KB_TO_GB(x) (((uint_t)(x)) >> 20)

#define MB_TO_B(x) (((uint_t)(x)) << 20)
#define MB_TO_KB(x) (((uint_t)(x)) << 10)
#define MB_TO_GB(x) (((uint_t)(x)) >> 10)

#define GB_TO_B(x) (((uint_t)(x)) << 30)
#define GB_TO_KB(x) (((uint_t)(x)) << 20)
#define GB_TO_MB(x) (((uint_t)(x)) << 10)

// potetially useful compiler attributes
#define ATTR_ALIGNED(x) __attribute__((__aligned__(x)))
#define ATTR_PACKED __attribute__((__packed__))
#define ATTR_UNUSED __attribute__((__unused__))

/*
** Utility macros
*/

//
// macros to clear data structures
//
// these are usable for clearing single-valued data items (e.g.,
// a PCB, etc.)
#define CLEAR(v) memclr(&v, sizeof(v))
#define CLEAR_PTR(p) memclr(p, sizeof(*p))

//
// macros for access registers and system call arguments
//

// REG(pcb,x) -- access a specific register in a process context
#define REG(pcb, x) ((pcb)->context->x)

// RET(pcb) -- access return value register in a process context
#define RET(pcb) ((pcb)->context->eax)

// ARG(pcb,n) -- access argument #n from the indicated process
//
// ARG(pcb,0) --> return address
// ARG(pcb,1) --> first parameter
// ARG(pcb,2) --> second parameter
// etc.
//
// ASSUMES THE STANDARD 32-BIT ABI, WITH PARAMETERS PUSHED ONTO THE
// STACK.  IF THE PARAMETER PASSING MECHANISM CHANGES, SO MUST THIS!
#define ARG(pcb, n) (((uint32_t *)(((pcb)->context) + 1))[(n)])

/*
** Types
*/

/*
** Globals
*/

// general-purpose character buffer
extern char b256[256];

// buffer for use by PANIC() macro
extern char b512[512];

// kernel stack
extern uint8_t kstack[SZ_KSTACK];

/*
** Prototypes
*/

#endif /* !ASM_SRC */

#endif
