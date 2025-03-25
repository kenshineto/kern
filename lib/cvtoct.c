/**
** @file	cvtoct.c
**
** @author	Numerous CSCI-452 classes
**
** @brief	C implementations of common library functions
*/

#ifndef CVTOCT_SRC_INC
#define CVTOCT_SRC_INC

#include <common.h>

#include <lib.h>

/**
** cvtoct(buf,value)
**
** convert a 32-bit unsigned value into a mininal-length (up to
** 11-character) NUL-terminated character string
**
** @param buf   Destination buffer
** @param value Value to convert
**
** @return The number of characters placed into the buffer
**          (not including the NUL)
**
** NOTE:  assumes buf is large enough to hold the resulting string
*/
int cvtoct( char *buf, uint32_t value ) {
	int i;
	int chars_stored = 0;
	char *bp = buf;
	uint32_t val;

	val = ( value & 0xc0000000 );
	val >>= 30;
	for( i = 0; i < 11; i += 1 ){

		if( i == 10 || val != 0 || chars_stored ) {
			chars_stored = 1;
			val &= 0x7;
			*bp++ = val + '0';
		}
		value <<= 3;
		val = ( value & 0xe0000000 );
		val >>= 29;
	}
	*bp = '\0';

	return bp - buf;
}

#endif
