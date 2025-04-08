/**
** @file debug.h
**
** @author Numerous CSCI-452 classes
**
** Debugging macros and constants.
**
*/

#ifndef DEBUG_H_
#define DEBUG_H_

// Standard system headers

#include <cio.h>
#include <support.h>

// Kernel library

#include <lib.h>

#ifndef ASM_SRC

/*
** Start of C-only definitions
*/

/*
** General function entry/exit announcements
*/

#ifdef ANNOUNCE_ENTRY
// Announce that we have entered a kernel function
// usage: ENTERING( "name" ), EXITING( "name" )
// currently, these do not use the macro parameter, but could be
// modified to do so; instead, we use the __func__ CPP pseudo-macro
// to get the function name
#define ENTERING(n)    do { cio_puts( " enter " __func__ ); } while(0)
#define EXITING(n)     do { cio_puts( " exit "  __func__ ); } while(0)
#else
#define ENTERING(m)    // do nothing
#define EXITING(m)     // do nothing
#endif

/*
** Console messages when error conditions are noted.
*/

// Warning messages to the console
// m: message (condition, etc.)
#define WARNING(m)  do { \
		cio_printf( "\n** %s (%s @ %d): ", __func__, __FILE__, __LINE__ ); \
		cio_puts( m ); \
		cio_putchar( '\n' ); \
	} while(0)

// Panic messages to the console
// n: severity level
// m: message (condition, etc.)
#define PANIC(n,m)  do { \
		sprint( b512, "%s (%s @ %d), %d: %s\n", \
				__func__, __FILE__, __LINE__, n, # m ); \
		kpanic( b512 ); \
	} while(0)

/*
** Assertions are categorized by the "sanity level" being used in this
** compilation; each only triggers a fault if the sanity level is at or
** above a specific value.  This allows selective enabling/disabling of
** debugging checks.
**
** The sanity level is set during compilation with the CPP macro
** "SANITY".  A sanity level of 0 disables conditional assertions,
** but not the basic assert() version.
*/

#ifndef SANITY
// default sanity check level: check everything!
#define SANITY  9999
#endif

// Always-active assertions
#define assert(x)   if( !(x) ) { PANIC(0,x); }

// only provide these macros if the sanity check level is positive

#if SANITY > 0

#define	assert1(x)	if( SANITY >= 1 && !(x) ) { PANIC(1,x); }
#define	assert2(x)	if( SANITY >= 2 && !(x) ) { PANIC(2,x); }
#define	assert3(x)	if( SANITY >= 3 && !(x) ) { PANIC(3,x); }
#define	assert4(x)	if( SANITY >= 4 && !(x) ) { PANIC(4,x); }
// arbitrary sanity level
#define	assertN(n,x)	if( SANITY >= (n) && !(x) ) { PANIC(n,x); }

#else

#define	assert1(x)		// do nothing
#define	assert2(x)		// do nothing
#define	assert3(x)		// do nothing
#define	assert4(x)		// do nothing
#define	assertN(n,x)	// do nothing

#endif /* SANITY > 0 */

/*
** Tracing options are enabled by defining one or more of the T_
** macros described in the Makefile.
**
** To add a tracing option:
**
**  1) Pick a short name for it (e.g., "PCB", "VM", ...)
**  2) At the end of this list, add code like this, with "name"
**     replaced by your short name, and "nnnnnnnn" replaced by a
**     unique bit that will designate this tracing option:
**
**        #ifdef T_name
**        #define TRname     0xnnnnnnnn
**        #else
**        #define TRname     0
**        #endif
**
**    Use the next bit position following the one in last list entry.
**  3) Add this to the end of the "TRACE" macro definition:
**
**        | TRname
**
**  4) In the list of "TRACING_*" macros, add one for your option
**     (using a name that might be more descriptive) in the 'then' clause:
**
**        #define TRACING_bettername  ((TRACE & TRname) != 0)
**
**  5) Also add a "null" version in the 'else' clause:
**
**        #define TRACING_bettername  0
**
**  6) Maybe add your T_name choice to the Makefile with an explanation
**     on the off chance you want anyone else to be able to understand
**     what it's used for. :-)
**
** We're making CPP work for its pay with this file.
*/

// 2^0 bit
#ifdef T_PCB
#define TRPCB           0x00000001
#else
#define TRPCB           0
#endif

#ifdef T_VM
#define TRVM            0x00000002
#else
#define TRVM            0
#endif

#ifdef T_QUE
#define TRQUEUE         0x00000004
#else
#define TRQUEUE         0
#endif

#ifdef T_SCH
#define TRSCHED         0x00000008
#else
#define TRSCHED         0
#endif

// 2^4 bit
#ifdef T_DSP
#define TRDISP          0x00000010
#else
#define TRDISP          0
#endif

#ifdef T_SCALL
#define TRSYSCALLS      0x00000020
#else
#define TRSYSCALLS      0
#endif

#ifdef T_SRET
#define TRSYSRETS       0x00000040
#else
#define TRSYSRETS       0
#endif

#ifdef T_EXIT
#define TREXIT          0x00000080
#else
#define TREXIT          0
#endif

// 2^8 bit
#ifdef T_INIT
#define TRINIT          0x00000100
#else
#define TRINIT          0
#endif

#ifdef T_KM
#define TRKMEM          0x00000200
#else
#define TRKMEM          0
#endif

#ifdef T_KMFR
#define TRKMEM_F        0x00000400
#else
#define TRKMEM_F        0
#endif

#ifdef T_KMIN
#define TRKMEM_I        0x00000800
#else
#define TRKMEM_I        0
#endif

// 2^12 bit
#ifdef T_FORK
#define TRFORK          0x00001000
#else
#define TRFORK          0
#endif

#ifdef T_EXEC
#define TREXEC          0x00002000
#else
#define TREXEC          0
#endif

#ifdef T_SIO
#define TRSIO_STAT      0x00004000
#else
#define TRSIO_STAT      0
#endif

#ifdef T_SIOR
#define TRSIO_RD        0x00008000
#else
#define TRSIO_RD        0
#endif

// 2^16 bit
#ifdef T_SIOW
#define TRSIO_WR        0x00010000
#else
#define TRSIO_WR        0
#endif

#ifdef T_USER
#define TRUSER          0x00020000
#else
#define TRUSER          0
#endif

#ifdef T_ELF
#define TRELF           0x00040000
#else
#define TRELF           0
#endif

// 13 bits remaining for tracing options
// next available bit: 0x00080000

#define TRACE (TRDISP | TREXIT | TRINIT | TRKMEM | TRKMEM_F | TRKMEM_I | TRPCB | TRQUEUE | TRSCHED | TREXEC | TRSIO_RD | TRSIO_STAT | TRSIO_WR | TRFORK | TRVM | TRSYSCALLS | TRSYSRETS | TRUSER | TRELF)

#if TRACE > 0

// compile-time expressions for testing trace options
// usage:  #if TRACING_thing
#define TRACING_PCB             ((TRACE & TRPCB) != 0)
#define TRACING_VM              ((TRACE & TRVM) != 0)
#define TRACING_QUEUE           ((TRACE & TRQUEUE) != 0)
#define TRACING_SCHED           ((TRACE & TRSCHED) != 0)
#define TRACING_DISPATCH        ((TRACE & TRDISPATCH) != 0)
#define TRACING_SYSCALLS        ((TRACE & TRSYSCALLS) != 0)
#define TRACING_SYSRETS         ((TRACE & TRSYSRETS) != 0)
#define TRACING_EXIT            ((TRACE & TREXIT) != 0)
#define TRACING_INIT            ((TRACE & TRINIT) != 0)
#define TRACING_KMEM            ((TRACE & TRKMEM) != 0)
#define TRACING_KMEM_FREELIST   ((TRACE & TRKMEM_F) != 0)
#define TRACING_KMEM_INIT       ((TRACE & TRKMEM_I) != 0)
#define TRACING_FORK            ((TRACE & TRFORK) != 0)
#define TRACING_EXEC            ((TRACE & TREXEC) != 0)
#define TRACING_SIO_STAT        ((TRACE & TRSIO_STAT) != 0)
#define TRACING_SIO_ISR         ((TRACE & TRSIO_ISR) != 0)
#define TRACING_SIO_RD          ((TRACE & TRSIO_RD) != 0)
#define TRACING_SIO_WR          ((TRACE & TRSIO_WR) != 0)
#define TRACING_USER            ((TRACE & TRUSER) != 0)
#define TRACING_ELF             ((TRACE & TRELF) != 0)

// more generic tests
#define TRACING_SOMETHING       (TRACE != 0)

#else

// TRACE == 0, so just define these all as "false"

#define TRACING_PCB             0
#define TRACING_STACK           0
#define TRACING_QUEUE           0
#define TRACING_SCHED           0
#define TRACING_DISPATCH        0
#define TRACING_SYSCALLS        0
#define TRACING_SYSRET          0
#define TRACING_EXIT            0
#define TRACING_INIT            0
#define TRACING_KMEM            0
#define TRACING_KMEM_FREELIST   0
#define TRACING_KMEM_INIT       0
#define TRACING_FORK            0
#define TRACING_EXEC            0
#define TRACING_SI_STAT         0
#define TRACING_SIO_ISR         0
#define TRACING_SIO_RD          0
#define TRACING_SIO_WR          0
#define TRACING_USER            0
#define TRACING_ELF             0

#define TRACING_SOMETHING       0

#endif /* TRACE */

#endif /* !ASM_SRC */

#endif
