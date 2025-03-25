/**
** @file	strlen.c
**
** @author	Numerous CSCI-452 classes
**
** @brief	C implementations of common library functions
*/

#ifndef STRLEN_SRC_INC
#define STRLEN_SRC_INC

#include <common.h>

#include <lib.h>

/**
** strlen(str) - return length of a NUL-terminated string
**
** @param str The string to examine
**
** @return The length of the string, or 0
*/
uint32_t strlen( register const char *str ) {
	register uint32_t len = 0;

	while( *str++ ) {
		++len;
	}

	return( len );
}
#endif
