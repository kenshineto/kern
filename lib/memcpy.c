/**
** @file	memcpy.c
**
** @author	Numerous CSCI-452 classes
**
** @brief	C implementations of common library functions
*/

#ifndef MEMCPY_SRC_INC
#define MEMCPY_SRC_INC

#include <common.h>

#include <lib.h>

/**
** memcpy(dst,src,len)
**
** Copy a block from one place to another
**
** May not correctly deal with overlapping buffers
**
** @param dst   Destination buffer
** @param src   Source buffer
** @param len   Buffer size (in bytes)
*/
void memcpy( void *dst, register const void *src, register uint32_t len ) {
	register uint8_t *dest = dst;
	register const uint8_t *source = src;

	/*
	** We could speed this up by unrolling it and copying
	** words at a time (instead of bytes).
	*/

	while( len-- ) {
		*dest++ = *source++;
	}
}

#endif
