#include <common.h>

/**
** User function Z:	 exit, sleep, write, getpid
**
** Prints its ID, then records PID and PPID, loops printing its ID,
** and finally re-gets PPID for comparison.  Yields after every second
** ID print in the loop.
**
** This code is used as a handy "spawn me" test routine; it is spawned
** by several of the standard test processes.
**
** Invoked as:	userZ  x  [ n ]
**	 where x is the ID character
**		   n is the iteration count (defaults to 10)
*/

USERMAIN( main ) {
	int count = 10;	  // default iteration count
	char ch = 'z';	  // default character to print
	char buf[128];

	// process the command-line arguments
	switch( argc ) {
	case 3:	count = str2int( argv[2], 10 );
			// FALL THROUGH
	case 2:	ch = argv[1][0];
			break;
	default:
			sprint( buf, "?: argc %d, args: ", argc );
			cwrites( buf );
			for( int i = 0; i <= argc; ++i ) {
				sprint( buf, " %s", argv[argc] ? argv[argc] : "(null)" );
				cwrites( buf );
			}
			cwrites( "\n" );
	}

	// announce our presence
	int pid = getpid();
	sprint( buf, " %c[%d]", ch, pid );
	swrites( buf );

	// iterate for a while; occasionally yield the CPU
	for( int i = 0; i < count ; ++i ) {
		sprint( buf, " %c[%d]", ch, i );
		swrites( buf );
		DELAY(STD);
		if( i & 1 ) {
			sleep( 0 );
		}
	}

	exit( 0 );

	return( 42 );  // shut the compiler up!
}
