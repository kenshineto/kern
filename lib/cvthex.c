/**
** @file	cvthex.c
**
** @author	Numerous CSCI-452 classes
**
** @brief	C implementations of common library functions
*/

#ifndef CVTHEX_SRC_INC
#define CVTHEX_SRC_INC

#include <common.h>

#include <lib.h>

/**
** cvthex(buf,value)
**
** convert a 32-bit unsigned value into a minimal-length (up to
** 8-character) NUL-terminated character string
**
** @param buf    Destination buffer
** @param value  Value to convert
**
** @return The number of characters placed into the buffer
**          (not including the NUL)
**
** NOTE:  assumes buf is large enough to hold the resulting string
*/
int cvthex( char *buf, uint32_t value ) {
	const char hexdigits[] = "0123456789ABCDEF";
	int chars_stored = 0;

	for( int i = 0; i < 8; i += 1 ) {
		uint32_t val = value & 0xf0000000;
		if( chars_stored || val != 0 || i == 7 ) {
			++chars_stored;
			val = (val >> 28) & 0xf;
			*buf++ = hexdigits[val];
		}
		value <<= 4;
	}

	*buf = '\0';

	return( chars_stored );
}

#endif
