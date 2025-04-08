/**
** @file	defs.h
**
** @author	Warren R. Carithers
**
** @brief	Common definitions.
**
** This header file defines things which are neede by all
** parts of the system (OS and user levels).
**
** Things which are kernel-specific go in the kdefs.h file;
** things which are user-specific go in the udefs.h file.
** The correct one of these will be automatically included
** at the end of this file.
*/

#ifndef DEFS_H_
#define DEFS_H_

/*
** General (C and/or assembly) definitions
**
** This section of the header file contains definitions that can be
** used in either C or assembly-language source code.
*/

// NULL pointer value
//
// we define this the traditional way so that
// it's usable from both C and assembly

#define NULL                0

// predefined i/o channels

#define CHAN_CIO            0
#define CHAN_SIO            1

// maximum allowable number of command-line arguments
#define MAX_ARGS            10

// sizes of various things
#define NUM_1KB           0x00000400    // 2^10
#define NUM_4KB           0x00001000    // 2^12
#define NUM_1MB           0x00100000    // 2^20
#define NUM_4MB           0x00400000    // 2^22
#define NUM_1GB           0x40000000    // 2^30
#define NUM_2GB           0x80000000    // 2^31
#define NUM_3GB           0xc0000000

#ifndef ASM_SRC

/*
** Start of C-only definitions
**
** Anything that should not be visible to something other than
** the C compiler should be put here.
*/

/*
** System error codes
**
** These can be returned to both system functions
** and to user system calls.
*/
	// success!
#define SUCCESS             (0)
#   define E_SUCCESS        SUCCESS
	// generic "something went wrong"
#define E_FAILURE           (-1)
	// specific failure reasons
#define E_BAD_PARAM         (-2)
#define E_BAD_CHAN          (-3)
#define E_NO_CHILDREN       (-4)
#define	E_NO_MEMORY         (-5)
#define	E_NOT_FOUND         (-6)
#define E_NO_PROCS          (-7)

/*
** These error codes are internal to the OS.
*/
#define E_EMPTY_QUEUE       (-100)
#define E_NO_PCBS           (-101)
#define	E_NO_PTE            (-102)
#define E_LOAD_LIMIT        (-103)

// exit status values
#define EXIT_SUCCESS        (0)
#define EXIT_FAILURE        (-1)
#define EXIT_KILLED         (-101)
#define EXIT_BAD_SYSCALL    (-102)

/*
** Process priority values
*/
enum priority_e {
	PRIO_HIGH, PRIO_STD, PRIO_LOW, PRIO_DEFERRED
	// sentinel
	, N_PRIOS
};

// halves of various data sizes

#define UI16_UPPER		0xff00
#define UI16_LOWER		0x00ff

#define UI32_UPPER		0xffff0000
#define UI32_LOWER		0x0000ffff

#define UI64_UPPER		0xffffffff00000000LL
#define UI64_LOWER		0x00000000ffffffffLL

// Simple conversion pseudo-functions usable by everyone

// convert seconds to ms
#define SEC_TO_MS(n)	((n) * 1000)

#endif	/* !ASM_SRC */

/*
** Level-specific definitions
*/
#ifdef KERNEL_SRC
#include <kdefs.h>
#else
#include <udefs.h>
#endif  /* KERNEL_SRC */

#endif
