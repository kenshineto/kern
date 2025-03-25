/**
** @file	pad.c
**
** @author	Numerous CSCI-452 classes
**
** @brief	C implementations of common library functions
*/

#ifndef PAD_SRC_INC
#define PAD_SRC_INC

#include <common.h>

#include <lib.h>

/**
** pad(dst,extra,padchar) - generate a padding string
**
** @param dst     Pointer to where the padding should begin
** @param extra   How many padding bytes to add
** @param padchar What character to pad with
**
** @return Pointer to the first byte after the padding
**
** NOTE: does NOT NUL-terminate the buffer
*/
char *pad( char *dst, int extra, int padchar ) {
	while( extra > 0 ){
		*dst++ = (char) padchar;
		extra -= 1;
	}
	return dst;
}

#endif
