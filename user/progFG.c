#include <common.h>

/**
** User function main #3:  exit, sleep, write
**
** Prints its ID, then loops N times sleeping and printing, then exits.
**
** Invoked as:  main3  x  n  s
**	 where x is the ID character
**		   n is the iteration count
**		   s is the sleep time in seconds
*/

USERMAIN( main ) {
	char ch = '3';	// default character to print
	int nap = 10;	// default sleep time
	int count = 30;	// iteration count
	char buf[128];

	// process the command-line arguments
	switch( argc ) {
	case 4:	nap = str2int( argv[3], 10 );
			// FALL THROUGH
	case 3:	count = str2int( argv[2], 10 );
			// FALL THROUGH
	case 2:	ch = argv[1][0];
			break;
	default:
			sprint( buf, "main3: argc %d, args: ", argc );
			cwrites( buf );
			for( int i = 0; i <= argc; ++i ) {
				sprint( buf, " %s", argv[argc] ? argv[argc] : "(null)" );
				cwrites( buf );
			}
			cwrites( "\n" );
	}

	// announce our presence
	int n = swritech( ch );
	if( n != 1 ) {
		sprint( buf, "=== %c, write #1 returned %d\n", ch, n );
		cwrites( buf );
	}

	write( CHAN_SIO, &ch, 1 );

	for( int i = 0; i < count ; ++i ) {
		sleep( SEC_TO_MS(nap) );
		write( CHAN_SIO, &ch, 1 );
	}

	exit( 0 );

	return( 42 );  // shut the compiler up!
}
