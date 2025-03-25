/**
** @file	strcat.c
**
** @author	Numerous CSCI-452 classes
**
** @brief	C implementations of common library functions
*/

#ifndef STRCAT_SRC_INC
#define STRCAT_SRC_INC

#include <common.h>

#include <lib.h>

/**
** strcat(dst,src) - append one string to another
**
** @param dst The destination buffer
** @param src The source buffer
**
** @return The dst parameter
**
** NOTE:  assumes dst is large enough to hold the resulting string
*/
char *strcat( register char *dst, register const char *src ) {
	register char *tmp = dst;

	while( *dst )  // find the NUL
		++dst;

	while( (*dst++ = *src++) )  // append the src string
		;

	return( tmp );
}

#endif
