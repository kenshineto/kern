/*
** @file    klib.h
**
** @author  Warren R. Carithers
**
** Additional support functions for the kernel.
*/

#ifndef KLIB_H_
#define KLIB_H_

#include <common.h>

#ifndef ASM_SRC

#include <x86/ops.h>

/**
** Name:    put_char_or_code( ch )
**
** Description: Prints a character on the console, unless it
** is a non-printing character, in which case its hex code
** is printed
**
** @param ch    The character to be printed
*/
void put_char_or_code( int ch );

/**
** Name:    backtrace
**
** Perform a simple stack backtrace. Could be augmented to use the
** symbol table to print function/variable names, etc., if so desired.
**
** @param ebp   Initial EBP to use
** @param args  Number of function argument values to print
*/
void backtrace( uint32_t *ebp, uint_t args );

/**
** Name:	kpanic
**
** Kernel-level panic routine
**
** usage:  kpanic( msg )
**
** Prefix routine for panic() - can be expanded to do other things
** (e.g., printing a stack traceback)
**
** @param msg[in]  String containing a relevant message to be printed,
**                 or NULL
*/
void kpanic( const char *msg );

#endif  /* !ASM_SRC */

#endif
