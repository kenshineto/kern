/**
** @file	padstr.c
**
** @author	Numerous CSCI-452 classes
**
** @brief	C implementations of common library functions
*/

#ifndef PADSTR_SRC_INC
#define PADSTR_SRC_INC

#include <common.h>

#include <lib.h>

/**
** padstr(dst,str,len,width,leftadjust,padchar - add padding characters
**                                               to a string
**
** @param dst        The destination buffer
** @param str        The string to be padded
** @param len        The string length, or -1
** @param width      The desired final length of the string
** @param leftadjust Should the string be left-justified?
** @param padchar    What character to pad with
**
** @return Pointer to the first byte after the padded string
**
** NOTE: does NOT NUL-terminate the buffer
*/
char *padstr( char *dst, char *str, int len, int width,
				int leftadjust, int padchar ) {
	int extra;

	// determine the length of the string if we need to
	if( len < 0 ){
		len = strlen( str );
	}

	// how much filler must we add?
	extra = width - len;

	// add filler on the left if we're not left-justifying
	if( extra > 0 && !leftadjust ){
		dst = pad( dst, extra, padchar );
	}

	// copy the string itself
	for( int i = 0; i < len; ++i ) {
		*dst++ = str[i];
	}

	// add filler on the right if we are left-justifying
	if( extra > 0 && leftadjust ){
		dst = pad( dst, extra, padchar );
	}

	return dst;
}

#endif
