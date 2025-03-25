/**
** @file	cvtuns0.c
**
** @author	Numerous CSCI-452 classes
**
** @brief	C implementations of common library functions
*/

#ifndef CVTUNS0_SRC_INC
#define CVTUNS0_SRC_INC

#include <common.h>

#include <lib.h>

/**
** cvtuns0(buf,value) - local support routine for cvtuns()
**
** Convert a 32-bit unsigned value into a NUL-terminated character string
**
** @param buf    Result buffer
** @param value  Value to be converted
**
** @return Pointer to the first unused byte in the buffer
**
** NOTE:  assumes buf is large enough to hold the resulting string
*/
char *cvtuns0( char *buf, uint32_t value ) {
	uint32_t quotient;

	quotient = value / 10;
	if( quotient != 0 ){
		buf = cvtdec0( buf, quotient );
	}
	*buf++ = value % 10 + '0';
	return buf;
}

#endif
