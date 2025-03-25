/**
** @file	cvtdec.c
**
** @author	Numerous CSCI-452 classes
**
** @brief	C implementations of common library functions
*/

#ifndef CVTDEC_SRC_INC
#define CVTDEC_SRC_INC

#include <common.h>

#include <lib.h>

/**
** cvtdec(buf,value)
**
** convert a 32-bit signed value into a NUL-terminated character string
**
** @param buf    Destination buffer
** @param value  Value to convert
**
** @return The number of characters placed into the buffer
**          (not including the NUL)
**
** NOTE:  assumes buf is large enough to hold the resulting string
*/
int cvtdec( char *buf, int32_t value ) {
	char *bp = buf;

	if( value < 0 ) {
		*bp++ = '-';
		value = -value;
	}

	bp = cvtdec0( bp, value );
	*bp  = '\0';

	return( bp - buf );
}

#endif
