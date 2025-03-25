/**
** @file	strcmp.c
**
** @author	Numerous CSCI-452 classes
**
** @brief	C implementations of common library functions
*/

#ifndef STRCMP_SRC_INC
#define STRCMP_SRC_INC

#include <common.h>

#include <lib.h>

/**
** strcmp(s1,s2) - compare two NUL-terminated strings
**
** @param s1 The first source string
** @param s2 The second source string
**
** @return negative if s1 < s2, zero if equal, and positive if s1 > s2
*/
int strcmp( register const char *s1, register const char *s2 ) {

	while( *s1 != 0 && (*s1 == *s2) )
		++s1, ++s2;

	return( *s1 - *s2 );
}

#endif
