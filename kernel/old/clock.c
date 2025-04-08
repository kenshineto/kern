/**
** @file	clock.c
**
** @author	CSCI-452 class of 20245
**
** @brief	Clock module implementation
*/

#define KERNEL_SRC

#include <common.h>

#include <clock.h>
#include <procs.h>

#include <x86/arch.h>
#include <x86/pic.h>
#include <x86/pit.h>

/*
** PRIVATE DEFINITIONS
*/

/*
** PRIVATE DATA TYPES
*/

/*
** PRIVATE GLOBAL VARIABLES
*/

// pinwheel control variables
static uint32_t pinwheel;   // pinwheel counter
static uint32_t pindex;     // index into pinwheel string

/*
** PUBLIC GLOBAL VARIABLES
*/

// current system time
uint32_t system_time;

/*
** PRIVATE FUNCTIONS
*/

/**
** Name:	clk_isr
**
** The ISR for the clock
**
** @param vector    Vector number for the clock interrupt
** @param code      Error code (0 for this interrupt)
*/
static void clk_isr( int vector, int code ) {

	// spin the pinwheel

	++pinwheel;
	if( pinwheel == (CLOCK_FREQ / 10) ) {
		pinwheel = 0;
		++pindex;
		cio_putchar_at( 0, 0, "|/-\\"[ pindex & 3 ] );
	}

#if defined(SYSTEM_STATUS)
	// Periodically, dump the queue lengths and the SIO status (along
	// with the SIO buffers, if non-empty).
	//
	// Define the symbol SYSTEM_STATUS with a value equal to the desired
	// reporting frequency, in seconds.

	if( (system_time % SEC_TO_TICKS(SYSTEM_STATUS)) == 0 ) {
		cio_printf_at( 1, 0, " queues: R[%u] W[%u] S[%u] Z[%u] I[%u]   ",
			pcb_queue_length(ready),
			pcb_queue_length(waiting),
			pcb_queue_length(sleeping),
			pcb_queue_length(zombie),
			pcb_queue_length(sioread)
		);
	}
#endif

	// time marches on!
	++system_time;

	// wake up any sleeping processes whose time has come
	//
	// we give them preference over the current process when
	// it is scheduled again

	do {
		// if there isn't anyone in the sleep queue, we're done
		if( pcb_queue_empty(sleeping) ) {
			break;
		}

		// peek at the first member of the queue
		pcb_t *tmp = pcb_queue_peek( sleeping );
		assert( tmp != NULL );

		// the sleep queue is sorted in ascending order by wakeup
		// time, so we know that the retrieved PCB's wakeup time is
		// the earliest of any process on the sleep queue; if that
		// time hasn't arrived yet, there's nobody left to awaken

		if( tmp->wakeup > system_time ) {
			break;
		}

		// OK, we need to wake this process up
		assert( pcb_queue_remove(sleeping,&tmp) == SUCCESS );
		schedule( tmp );
	} while( 1 );

	// next, we decrement the current process' remaining time
	current->ticks -= 1;

	// has it expired?
	if( current->ticks < 1 ) {
		// yes! reschedule it
		schedule( current );
		current = NULL;
		// and pick a new process
		dispatch();
	}

	// tell the PIC we're done
	outb( PIC1_CMD, PIC_EOI );
}

/*
** PUBLIC FUNCTIONS
*/

/**
** Name:  clk_init
**
** Initializes the clock module
**
*/
void clk_init( void ) {

#if TRACING_INIT
	cio_puts( " Clock" );
#endif

	// start the pinwheel
	pinwheel = (CLOCK_FREQ / 10) - 1;
	pindex = 0;

	// return to the dawn of time
	system_time = 0;

	// configure the clock
	uint32_t divisor = PIT_FREQ / CLOCK_FREQ;
	outb( PIT_CONTROL_PORT, PIT_0_LOAD | PIT_0_SQUARE );
	outb( PIT_0_PORT, divisor & 0xff );        // LSB of divisor
	outb( PIT_0_PORT, (divisor >> 8) & 0xff ); // MSB of divisor

	// register the second-stage ISR
	install_isr( VEC_TIMER, clk_isr );
}
