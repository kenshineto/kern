/**
** @file	strcpy.c
**
** @author	Numerous CSCI-452 classes
**
** @brief	C implementations of common library functions
*/

#ifndef STRCPY_SRC_INC
#define STRCPY_SRC_INC

#include <common.h>

#include <lib.h>

/**
** strcpy(dst,src) - copy a NUL-terminated string
**
** @param dst The destination buffer
** @param src The source buffer
**
** @return The dst parameter
**
** NOTE:  assumes dst is large enough to hold the copied string
*/
char *strcpy( register char *dst, register const char *src ) {
	register char *tmp = dst;

	while( (*dst++ = *src++) )
		;

	return( tmp );
}

#endif
