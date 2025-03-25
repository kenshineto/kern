/**
** @file	bound.c
**
** @author	Numerous CSCI-452 classes
**
** @brief	C implementations of common library functions
*/

#ifndef BOUND_SRC_INC
#define BOUND_SRC_INC

#include <common.h>

#include <lib.h>

/**
** bound(min,value,max)
**
** This function confines an argument within specified bounds.
**
** @param min    Lower bound
** @param value  Value to be constrained
** @param max    Upper bound
**
** @return The constrained value
*/
uint32_t bound( uint32_t min, uint32_t value, uint32_t max ) {
	if( value < min ){
		value = min;
	}
	if( value > max ){
		value = max;
	}
	return value;
}

#endif
