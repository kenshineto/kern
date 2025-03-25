/**
** @file	cvtdec0.c
**
** @author	Numerous CSCI-452 classes
**
** @brief	C implementations of common library functions
*/

#ifndef CVTDEC0_SRC_INC
#define CVTDEC0_SRC_INC

#include <common.h>

#include <lib.h>

/**
** cvtdec0(buf,value) - local support routine for cvtdec()
**
** convert a 32-bit unsigned integer into a NUL-terminated character string
**
** @param buf    Destination buffer
** @param value  Value to convert
**
** @return The number of characters placed into the buffer
**          (not including the NUL)
**
** NOTE:  assumes buf is large enough to hold the resulting string
*/
char *cvtdec0( char *buf, int value ) {
	int quotient;

	quotient = value / 10;
	if( quotient < 0 ) {
		quotient = 214748364;
		value = 8;
	}
	if( quotient != 0 ) {
		buf = cvtdec0( buf, quotient );
	}
	*buf++ = value % 10 + '0';
	return buf;
}

#endif
