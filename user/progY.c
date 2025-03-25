#include <common.h>

/**
** User function Y:	 exit, sleep, write, getpid
**
** Reports its PID, then iterates N times printing 'Yx' and
** sleeping for one second, then exits.
**
** Invoked as:	userY  x  [ n ]
**	 where x is the ID character
**		   n is the iteration count (defaults to 10)
*/

USERMAIN( main ) {
	int count = 10;	  // default iteration count
	char ch = 'y';	  // default character to print
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

	// report our presence
	int pid = getpid();
	sprint( buf, " %c[%d]", ch, pid );
	swrites( buf );

	for( int i = 0; i < count ; ++i ) {
		swrites( buf );
		DELAY(STD);
		sleep( SEC_TO_MS(1) );
	}

	exit( 0 );

	return( 42 );  // shut the compiler up!
}
