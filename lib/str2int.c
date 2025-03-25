/**
** @file	str2int.c
**
** @author	Numerous CSCI-452 classes
**
** @brief	C implementations of common library functions
*/

#ifndef STR2INT_SRC_INC
#define STR2INT_SRC_INC

#include <common.h>

#include <lib.h>

/**
** str2int(str,base) - convert a string to a number in the specified base
**
** @param str   The string to examine
** @param base  The radix to use in the conversion
**
** @return The converted integer
*/
int str2int( register const char *str, register int base ) {
	register int num = 0;
	register char bchar = '9';
	int sign = 1;

	// check for leading '-'
	if( *str == '-' ) {
		sign = -1;
		++str;
	}

	if( base != 10 ) {
		bchar = '0' + base - 1;
	}

	// iterate through the characters
	while( *str ) {
		if( *str < '0' || *str > bchar )
			break;
		num = num * base + *str - '0';
		++str;
	}

	// return the converted value
	return( num * sign );
}

#endif
