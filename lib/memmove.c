/**
** @file	memmove.c
**
** @author	Numerous CSCI-452 classes
**
** @brief	C implementations of common library functions
*/

#ifndef MEMMOVE_SRC_INC
#define MEMMOVE_SRC_INC

#include <common.h>

#include <lib.h>

/**
** memmove(dst,src,len)
**
** Copy a block from one place to another. Deals with overlapping
** buffers.
**
** @param dst   Destination buffer
** @param src   Source buffer
** @param len   Buffer size (in bytes)
*/
void memmove(void *dst, const void *src, register uint32_t len)
{
	register uint8_t *dest = dst;
	register const uint8_t *source = src;

	/*
	** We could speed this up by unrolling it and copying
	** words at a time (instead of bytes).
	*/

	if (source < dest && (source + len) > dest) {
		source += len;
		dest += len;
		while (len-- > 0) {
			*--dest = *--source;
		}
	} else {
		while (len--) {
			*dest++ = *source++;
		}
	}
}

#endif
