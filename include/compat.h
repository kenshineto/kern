/**
** @file	compat.h
**
** @author	Warren R. Carithers
**
** @brief	Compatibility definitions for standard modules.
**
** These definitions are here to simplify the integration
** of some pre-written modules into the 452 baseline system.
** This is used primarily for the 'kmem' and 'sio' modules.
**
** We use CPP symbols and not actual data types for things here,
** as this means we don't need to include any other header files
** into this file.  This helps get around "include loops" (e.g.,
** a.h includes b.h, which includes c.h, which includes a.h) when
** there are many interdependencies between source files.
*/

#ifndef COMPAT_H_
#define COMPAT_H_

#include <common.h>
#include <procs.h>

/*
** Section 1:  sized integer types
**
** Internally, we use standard names for "sized" integer types for
** simplicity.  If those disagree with the names used in the rest of
** the system, we take the opportunity to define our names here.
**
** To enable these, uncomment them, and place the apropriate
** existing type names in place of the '?' characters.
*/

// standard "sized integer" types
// #define int8_t       ?
// #define uint8_t      ?
// #define int16_t      ?
// #define uint16_t     ?
// #define int32_t      ?
// #define uint32_t     ?
// #define int64_t      ?
// #define uint64_t     ?
// #define bool_t       ?

/*
** Section 2:  other types
**
** Add type definitions here as needed.
**
** Note:  we do not include the PCB and Queue declarations
** here because we don't actually need them in this header
** file - we're only defining CPP macros.  Whatever file
** uses these macros, however, must include the appropriate
** headers if it uses these macros.
**
** To enable these, uncomment them, and place the apropriate
** existing type names in place of the '?' characters.
*/

// type name for the PCB
#define PCBTYPE		pcb_t

// type name for our queue
#define QTYPE		pcb_queue_t

/*
** Section 3:  interface and behavior
**
** Include #define statements here as needed to define
** the names of functions and globals used in these modules
** in terms of the names used in the rest of the baseline.
**
** To enable these, uncomment them, and place the apropriate
** existing variable or function names in place of the '?' characters.
*/

// string functions
#define SLENGTH		strlen

// scheduler
#define SCHED		schedule

// dispatcher
#define DISPATCH	dispatch

/*
** blocked queue for reading processes
**
** Define this if we are blocking processes which try to
** read from the SIO when no characters are available.
** Its value should be the name of the globally-visible
** queue to be used.
*/
#define QNAME		sioread

#ifdef QNAME

// Only define these macros if we need to be able to create and
// manage a queue of things. It is expected that these will need
// to be customized based on the names and calling sequences of
// the appropriate functions.

// invoke the queue creation function
// examples:
//
// 	#define QCREATE(q)	do { 
// 		_que_create( &(q), NULL ); 
//		} while(0)
//
//	#define QCREATE(q)	// do nothing

#define	QCREATE(q)	// handled elsewhere for us

// check to see if the queue is empty
// examples:
//
//	#define QEMPTY(q)	queue_is_empty(q)
//	#define	QEMPTY(q)	(quene_length(q) > 0)
#define	QEMPTY(q)	pcb_queue_empty(q)

// this macro expands into code that removes a value from
// 'q' and places it into 'd'
#define QDEQUE(q,d)	do { \
		assert(pcb_queue_remove( (q), (pcb_t **) &(d) ) == SUCCESS ); \
	} while(0)

#endif /* QNAME */

#endif
