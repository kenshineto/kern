/**
** @file	blkmov.c
**
** @author	Numerous CSCI-452 classes
**
** @brief	C implementations of common library functions
*/

#ifndef BLKMOV_SRC_INC
#define BLKMOV_SRC_INC

#include <common.h>

#include <lib.h>

/**
** blkmov(dst,src,len)
**
** Copy a word-aligned block from src to dst. Deals with overlapping
** buffers.
**
** @param dst   Destination buffer
** @param src   Source buffer
** @param len   Buffer size (in bytes)
*/
void blkmov(void *dst, const void *src, register uint32_t len)
{
	// verify that the addresses are aligned and
	// the length is a multiple of four bytes
	if ((((uint32_t)dst) & 0x3) != 0 || (((uint32_t)src) & 0x3) != 0 ||
		(len & 0x3) != 0) {
		// something isn't aligned, so just use memmove()
		memmove(dst, src, len);
		return;
	}

	// everything is nicely aligned, so off we go
	register uint32_t *dest = dst;
	register const uint32_t *source = src;

	// now copying 32-bit values
	len /= 4;

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
