#include <common.h>

/**
** User function X:   exit, write, getpid
**
** Prints its PID at start and exit, iterates printing its character
** N times, and exits with a status of 12.
**
** Invoked as:  userX  x  n
**	 where x is the ID character
**		   n is the iteration count
*/

USERMAIN( main ) {
	int count = 20;	  // iteration count
	char ch = 'x';	  // default character to print
	char buf[128];

	// process the command-line arguments
	switch( argc ) {
	case 3:	count = str2int( argv[2], 10 );
			// FALL THROUGH
	case 2:	ch = argv[1][0];
			break;
	default:
			sprint( buf, "userX: argc %d, args: ", argc );
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

	for( int i = 0; i < count ; ++i ) {
		swrites( buf );
		DELAY(STD);
	}

	exit( 12 );

	return( 42 );  // shut the compiler up!
}
