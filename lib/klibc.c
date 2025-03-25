/*
** @file klibc.c
**
** @author  Warren R. Carithers
**
** Additional support functions for the kernel.
**
*/

#define KERNEL_SRC

#include <klib.h>
#include <cio.h>
#include <procs.h>
#include <support.h>

/**
** Name:    put_char_or_code( ch )
**
** Description: Prints a character on the console, unless it
** is a non-printing character, in which case its hex code
** is printed
**
** @param ch    The character to be printed
*/
void put_char_or_code( int ch ) {

	if( ch >= ' ' && ch < 0x7f ) {
		cio_putchar( ch );
	} else {
		cio_printf( "\\x%02x", ch );
	}
}

/**
** Name:    backtrace
**
** Perform a stack backtrace
**
** @param ebp   Initial EBP to use
** @param args  Number of function argument values to print
*/
void backtrace( uint32_t *ebp, uint_t args ) {

	cio_puts( "Trace:  " );
	if( ebp == NULL ) {
		cio_puts( "NULL ebp, no trace possible\n" );
		return;
	} else {
		cio_putchar( '\n' );
	}

	while( ebp != NULL ){

		// get return address and report it and EBP
		uint32_t ret = ebp[1];
		cio_printf( " ebp %08x ret %08x args", (uint32_t) ebp, ret );

		// print the requested number of function arguments
		for( uint_t i = 0; i < args; ++i ) {
			cio_printf( " [%u] %08x", i+1, ebp[2+i] );
		}
		cio_putchar( '\n' );

		// follow the chain
		ebp = (uint32_t *) *ebp;
	}
}

/**
** kpanic - kernel-level panic routine
**
** usage:  kpanic( msg )
**
** Prefix routine for panic() - can be expanded to do other things
** (e.g., printing a stack traceback)
**
** @param msg[in]  String containing a relevant message to be printed,
**				   or NULL
*/
void kpanic( const char *msg ) {

	cio_puts( "\n\n***** KERNEL PANIC *****\n\n" );

	if( msg ) {
		cio_printf( "%s\n", msg );
	}

	delay( DELAY_5_SEC );   // approximately

	// dump a bunch of potentially useful information

	// dump the contents of the current PCB
	pcb_dump( "Current", current, true );

	// dump the basic info about what's in the process table
	ptable_dump_counts();

	// dump information about the queues
	pcb_queue_dump( "R", ready, true );
	pcb_queue_dump( "W", waiting, true );
	pcb_queue_dump( "S", sleeping, true );
	pcb_queue_dump( "Z", zombie, true );
	pcb_queue_dump( "I", sioread, true );

	// perform a stack backtrace
	backtrace( (uint32_t *) r_ebp(), 3 );

	// could dump other stuff here, too

	panic( "KERNEL PANIC" );
}
