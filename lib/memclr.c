/**
** @file	memclr.c
**
** @author	Numerous CSCI-452 classes
**
** @brief	C implementations of common library functions
*/

#ifndef MEMCLR_SRC_INC
#define MEMCLR_SRC_INC

#include <common.h>

#include <lib.h>

/**
** memclr(buf,len)
**
** Initialize all bytes of a block of memory to zero
**
** @param buf    The buffer to initialize
** @param len    Buffer size (in bytes)
*/
void memclr( void *buf, register uint32_t len ) {
	register uint8_t *dest = buf;

	/*
	** We could speed this up by unrolling it and clearing
	** words at a time (instead of bytes).
	*/

	while( len-- ) {
			*dest++ = 0;
	}
}

#endif
