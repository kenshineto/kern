/**
** @file	sprint.c
**
** @author	Numerous CSCI-452 classes
**
** @brief	C implementations of common library functions
*/

#ifndef SPRINT_SRC_INC
#define SPRINT_SRC_INC

#include <common.h>

#include <lib.h>

/**
** sprint(dst,fmt,...) - formatted output into a string buffer
**
** @param dst The string buffer
** @param fmt Format string
**
** The format string parameter is followed by zero or more additional
** parameters which are interpreted according to the format string.
**
** NOTE:  assumes the buffer is large enough to hold the result string
**
** NOTE:  relies heavily on the x86 parameter passing convention
** (parameters are pushed onto the stack in reverse order as
** 32-bit values).
*/
void sprint( char *dst, char *fmt, ... ) {
	int32_t *ap;
	char buf[ 12 ];
	char ch;
	char *str;
	int leftadjust;
	int width;
	int len;
	int padchar;

	/*
	** Get characters from the format string and process them
	**
	** We use the "old-school" method of handling variable numbers
	** of parameters.  We assume that parameters are passed on the
	** runtime stack in consecutive longwords; thus, if the first
	** parameter is at location 'x', the second is at 'x+4', the
	** third at 'x+8', etc.  We use a pointer to a 32-bit thing
	** to point to the next "thing", and interpret it according
	** to the format string.
	*/
	
	// get the pointer to the first "value" parameter
	ap = (int *)(&fmt) + 1;

	// iterate through the format string
	while( (ch = *fmt++) != '\0' ){
		/*
		** Is it the start of a format code?
		*/
		if( ch == '%' ){
			/*
			** Yes, get the padding and width options (if there).
			** Alignment must come at the beginning, then fill,
			** then width.
			*/
			leftadjust = 0;
			padchar = ' ';
			width = 0;
			ch = *fmt++;
			if( ch == '-' ){
				leftadjust = 1;
				ch = *fmt++;
			}
			if( ch == '0' ){
				padchar = '0';
				ch = *fmt++;
			}
			while( ch >= '0' && ch <= '9' ){
				width *= 10;
				width += ch - '0';
				ch = *fmt++;
			}

			/*
			** What data type do we have?
			*/
			switch( ch ) {

			case 'c':  // characters are passed as 32-bit values
				ch = *ap++;
				buf[ 0 ] = ch;
				buf[ 1 ] = '\0';
				dst = padstr( dst, buf, 1, width, leftadjust, padchar );
				break;

			case 'd':
				len = cvtdec( buf, *ap++ );
				dst = padstr( dst, buf, len, width, leftadjust, padchar );
				break;

			case 's':
				str = (char *) (*ap++);
				dst = padstr( dst, str, -1, width, leftadjust, padchar );
				break;

			case 'x':
				len = cvthex( buf, *ap++ );
				dst = padstr( dst, buf, len, width, leftadjust, padchar );
				break;

			case 'o':
				len = cvtoct( buf, *ap++ );
				dst = padstr( dst, buf, len, width, leftadjust, padchar );
				break;

			case 'u':
				len = cvtuns( buf, *ap++ );
				dst = padstr( dst, buf, len, width, leftadjust, padchar );
				break;

			}
		} else {
			// no, it's just an ordinary character
			*dst++ = ch;
		}
	}

	// NUL-terminate the result
	*dst = '\0';
}

#endif
