/**
** @file	ops.h
**
** @author	Warren R. Carithers
**
** @brief	Inline escapes to assembly for efficiency
**
** Inspiration from:
**	Martins Mozeiko, https://gist.github.com/mmozeiko/f68ad2546bd6ab953315
**  MIT's xv6,       https://github.com/mit-pdos/xv6-public
**
** Note: normally, GCC doesn't inline unless the optimization level is
** over 1. This can be forced by adding 
**
**      __attribute__((always_inline))
**
** after the parameter list on each declaration. This is enabled by
** defining the compile-time CPP symbol FORCE_INLINING.
*/

#ifndef OPS_H_
#define OPS_H_

#include <common.h>

#ifndef ASM_SRC

// control "forced" inlining
#ifdef FORCE_INLINING
#define OPSINLINED    __attribute__((always_inline))
#else
#define OPSINLINED    /* no-op */
#endif  /* FORCE_INLINING */

/****************************
** Data movement
****************************/

/**
** Block move functions
**
** Variations:  movsb(), movsl(), movsq()
**
** Description:  Copy from source buffer to destination buffer
**
** @param dst  Destination buffer
** @param src  Source buffer
** @param len  Byte count
*/
static inline void
movsb( void* dst, const void* src, uint32_t len ) OPSINLINED
{
	__asm__ __volatile__( "cld; rep movsb"
			: "+D"(dst), "+S"(src), "+c"(len)
			: : "memory" );
}

static inline void
movsw( void* dst, const void* src, uint32_t len ) OPSINLINED
{
	__asm__ __volatile__( "cld; rep movsw"
			: "+D"(dst), "+S"(src), "+c"(len)
			: : "memory" );
}

static inline void
movsl( void* dst, const void* src, uint32_t len ) OPSINLINED
{
	__asm__ __volatile__( "cld; rep movsl"
			: "+D"(dst), "+S"(src), "+c"(len)
			: : "memory" );
}

static inline void
movsq( void* dst, const void* src, uint32_t len ) OPSINLINED
{
	__asm__ __volatile__( "cld; rep movsq"
			: "+D"(dst), "+S"(src), "+c"(len)
			: : "memory" );
}

/**
** Block store functions
**
** Variations:  stosb(), stosw(), stosl()
**
** Description:  Store a specific value into destination buffer
**
** @param dst  Destination buffer
** @param val  Data to copy
** @param len  Byte count
*/
static inline void
stosb( void *dst, uint8_t val, uint32_t len ) OPSINLINED
{
	__asm__ __volatile__( "cld; rep stosb"
			: "=D" (dst), "=c" (len)
			: "0" (dst), "1" (len), "a" (val)
			: "memory", "cc" );
}

static inline void
stosw( void *dst, uint16_t val, uint32_t len ) OPSINLINED
{
	__asm__ __volatile__( "cld; rep stos2"
			: "=D" (dst), "=c" (len)
			: "0" (dst), "1" (len), "a" (val)
			: "memory", "cc" );
}

static inline void
stosl( void *dst, uint32_t val, uint32_t len ) OPSINLINED
{
	__asm__ __volatile__( "cld; rep stosl"
			: "=D" (dst), "=c" (len)
			: "0" (dst), "1" (len), "a" (val)
			: "memory", "cc" );
}

/****************************
** Special register access
****************************/

/**
** Register read functions
**
** Variations:  r_cr0(), r_cr2(), r_cr3(), r_cr4(), r_eflags(),
**              r_ebp(), r_esp()
**
** Description:  Reads the register indicated by its name
**
** @return Contents of the register
*/
static inline uint32_t
r_cr0( void ) OPSINLINED
{
	uint32_t val;
	__asm__ __volatile__( "movl %%cr0,%0" : "=r" (val) );
	return val;
}

static inline uint32_t
r_cr2( void ) OPSINLINED
{
	uint32_t val;
	__asm__ __volatile__( "movl %%cr2,%0" : "=r" (val) );
	return val;
}

static inline uint32_t
r_cr3( void ) OPSINLINED
{
	uint32_t val;
	__asm__ __volatile__( "movl %%cr3,%0" : "=r" (val) );
	return val;
}

static inline uint32_t
r_cr4( void ) OPSINLINED
{
	uint32_t val;
	__asm__ __volatile__( "movl %%cr4,%0" : "=r" (val) );
	return val;
}

static inline uint32_t
r_eflags(void) OPSINLINED
{
	uint32_t val;
	__asm__ __volatile__( "pushfl; popl %0" : "=r" (val) );
	return val;
}

static inline uint32_t
r_ebp(void) OPSINLINED
{
	uint32_t val;
	__asm__ __volatile__( "movl %%ebp,%0" : "=r" (val) );
	return val;
}

static inline uint32_t
r_esp(void) OPSINLINED
{
	uint32_t val;
	__asm__ __volatile__( "movl %%esp,%0" : "=r" (val) );
	return val;
}

/**
** Register write functions
**
** Variations:  w_cr0(), w_cr2(), w_cr3(), w_cr4(), w_eflags()
**
** Description:  Writes a value into the CR indicated by its name
*/
static inline void
w_cr0( uint32_t val ) OPSINLINED
{
	__asm__ __volatile__( "movl %0,%%cr0" : : "r" (val) );
}

static inline void
w_cr2( uint32_t val ) OPSINLINED
{
	__asm__ __volatile__( "movl %0,%%cr2" : : "r" (val) );
}

static inline void
w_cr3( uint32_t val ) OPSINLINED
{
	__asm__ __volatile__( "movl %0,%%cr3" : : "r" (val) );
}

static inline void
w_cr4( uint32_t val ) OPSINLINED
{
	__asm__ __volatile__( "movl %0,%%cr4" : : "r" (val) );
}

static inline void
w_eflags(uint32_t eflags) OPSINLINED
{
	__asm__ __volatile__( "pushl %0; popfl" : : "r" (eflags) );
}

/**
** Descriptor table load functions
**
** Variations:  w_gdt(), w_idt()
**
** Description:  Load an address into the specified processor register
**
** @param addr  The value to be loaded into the register
*/
static inline void
w_gdt( void *addr ) OPSINLINED
{
	__asm__ __volatile__( "lgdt (%0)" : : "r" (addr) );
}

static inline void
w_idt( void *addr ) OPSINLINED
{
	__asm__ __volatile__( "lidt (%0)" : : "r" (addr) );
}

/**
** CPU ID access
**
** Description:  Retrieve CPUID information
**
** @param op  Value to be placed into %eax for the operation
** @param ap   Pointer to where %eax contents should be saved, or NULL
** @param bp   Pointer to where %ebx contents should be saved, or NULL
** @param cp   Pointer to where %ecx contents should be saved, or NULL
** @param dp   Pointer to where %edx contents should be saved, or NULL
*/
static inline void
cpuid( uint32_t op, uint32_t *ap, uint32_t *bp,
		uint32_t *cp, uint32_t *dp ) OPSINLINED
{
	uint32_t eax, ebx, ecx, edx;
	__asm__ __volatile__( "cpuid"
			: "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
			: "a" (op) );

	if( ap ) *ap = eax;
	if( bp ) *bp = ebx;
	if( cp ) *cp = ecx;
	if( dp ) *dp = edx;
}

/****************************
** TLB management
****************************/

/**
** TLB invalidation for one page
**
** Description:  Invalidate the TLB entry for an address
**
** @param addr  An address within the page to be flushed
*/
static inline void
invlpg( uint32_t addr ) OPSINLINED
{
	__asm__ __volatile__( "invlpg (%0)" : : "r" (addr) : "memory" );
}

/**
** TLB invalidation for all pages
**
** Description:  Flush all entries from the TLB
**
** We do this by changing CR3.
*/
static inline void
flushtlb( void ) OPSINLINED
{
	uint32_t cr3;
	__asm__ __volatile__( "movl %%cr3,%0" : "=r" (cr3) );
	__asm__ __volatile__( "movl %0,%%cr2" : : "r" (cr3) );
}

/****************************
** I/O instructions
****************************/

/**
** Name:	inN
**
** Variations:  inb(), inw(), inl()
**
** Description:  Read some amount of data from the supplied I/O port
**
** @param port  The i/o port to read from
**
** @return The data read from the specified port
*/
static inline uint8_t
inb( int port ) OPSINLINED
{
	uint8_t data;
	__asm__ __volatile__( "inb %w1,%0" : "=a" (data) : "d" (port) );
	return data;
}

static inline uint16_t
inw( int port ) OPSINLINED
{
	uint16_t data;
	__asm__ __volatile__( "inw %w1,%0" : "=a" (data) : "d" (port) );
	return data;
}

static inline uint32_t
inl( int port ) OPSINLINED
{
	uint32_t data;
	__asm__ __volatile__( "inl %w1,%0" : "=a" (data) : "d" (port) );
	return data;
}

/**
** Name:	outN
**
** Variations:  outb(), outw(), outl()
**
** Description:  Write some data to the specified I/O port
**
** @param port  The i/o port to write to
** @param data  The data to be written to the port
**
** @return The data read from the specified port
*/
static inline void
outb( int port, uint8_t data ) OPSINLINED
{
	__asm__ __volatile__( "outb %0,%w1" : : "a" (data), "d" (port) );
}

static inline void
outw( int port, uint16_t data ) OPSINLINED
{
	__asm__ __volatile__( "outw %0,%w1" : : "a" (data), "d" (port) );
}

static inline void
outl( int port, uint32_t data ) OPSINLINED
{
	__asm__ __volatile__( "outl %0,%w1" : : "a" (data), "d" (port) );
}

/****************************
** Miscellaneous instructions
****************************/

/**
** Name:	breakpoint
**
** Description:  Cause a breakpoint interrupt for debugging purposes
*/
static inline void
breakpoint( void ) OPSINLINED
{
	__asm__ __volatile__( "int3" );
}

/**
** Name:	get_ra
**
** Description:  Get the return address for the calling function
**              (i.e., where whoever called us will go back to)
**
** @return The address the calling routine will return to as a uint32_t
*/
static inline uint32_t
get_ra( void ) OPSINLINED
{
	uint32_t val;
	__asm__ __volatile__( "movl 4(%%ebp),%0" : "=r" (val) );
	return val;
}

/**
** Name:	ev_wait
**
** Description:  Pause until something happens
*/
static inline void
ev_wait( void ) OPSINLINED
{
	__asm__ __volatile__( "sti ; hlt" );
}


/**
** Name:	xchgl
**
** Description:  Perform an atomic exchange with memory
**
** @param addr  Memory location to be modified
** @param data  Data to exchange
**
** @return The old contents of the memory location
*/
static inline uint32_t
xchgl( volatile uint32_t *addr, uint32_t data ) OPSINLINED
{
	uint32_t old;

	// + indicates a read-modify-write operand
	__asm__ __volatile__( "lock; xchgl %0, %1"
		     : "+m" (*addr), "=a" (old)
		     : "1" (data)
		     : "cc");
	return old;
}

#endif  /* !ASM_SRC */

#endif
