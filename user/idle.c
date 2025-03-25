#include <common.h>

/**
** Idle process:  write, getpid, gettime, exit
**
** Reports itself, then loops forever delaying and printing a character.
** MUST NOT SLEEP, as it must always be available in the ready queue 
** when there is no other process to dispatch.
**
** Invoked as:	idle
*/

USERMAIN( main ) {
	// this is the character we will repeatedly print
	char ch = '.';

	// ignore the command-line arguments
	(void) argc;
	(void) argv;

	// get some current information
	uint_t pid = getpid();
	uint32_t now = gettime();
	enum priority_e prio = getprio();

	char buf[128];
	sprint( buf, "Idle [%d], started @ %u\n", pid, prio, now );
	cwrites( buf );
	
	// report our presence on the console
	cwrites( "Idle started\n" );

	write( CHAN_SIO, &ch, 1 );

	// idle() should never block - it must always be available
	// for dispatching when we need to pick a new current process

	for(;;) {
		DELAY(LONG);
		write( CHAN_SIO, &ch, 1 );
	}

	// we should never reach this point!
	now = gettime();
	sprint( buf, "Idle [%d] EXITING @ %u!?!?!\n", pid, now );
	cwrites( buf );

	exit( 1 );

	return( 42 );
}
