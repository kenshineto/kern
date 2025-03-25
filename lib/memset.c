/**
** @file	memset.c
**
** @author	Numerous CSCI-452 classes
**
** @brief	C implementations of common library functions
*/

#ifndef MEMSET_SRC_INC
#define MEMSET_SRC_INC

#include <common.h>

#include <lib.h>

/**
** memset(buf,len,value)
**
** initialize all bytes of a block of memory to a specific value
**
** @param buf    The buffer to initialize
** @param len    Buffer size (in bytes)
** @param value  Initialization value
*/
void memset( void *buf, register uint32_t len, register uint32_t value ) {
	register uint8_t *bp = buf;

	/*
	** We could speed this up by unrolling it and copying
	** words at a time (instead of bytes).
	*/

	while( len-- ) {
		*bp++ = value;
	}
}

#endif
