/**
** @file	cvtuns.c
**
** @author	Numerous CSCI-452 classes
**
** @brief	C implementations of common library functions
*/

#ifndef CVTUNS_SRC_INC
#define CVTUNS_SRC_INC

#include <common.h>

#include <lib.h>

/**
** cvtuns(buf,value)
**
** Convert a 32-bit unsigned value into a NUL-terminated character string
**
** @param buf    Result buffer
** @param value  Value to be converted
**
** @return Length of the resulting buffer
**
** NOTE:  assumes buf is large enough to hold the resulting string
*/
int cvtuns( char *buf, uint32_t value ) {
	char    *bp = buf;

	bp = cvtuns0( bp, value );
	*bp = '\0';

	return bp - buf;
}

#endif
