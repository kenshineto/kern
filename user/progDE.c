#include <common.h>

/**
** User function main #2:  write
**
** Prints its ID, then loops N times delaying and printing, then returns
** without calling exit(). Verifies the return byte count from each call
** to write().
**
** Invoked as:  main2  x  n
**	 where x is the ID character
**		   n is the iteration count
*/

USERMAIN( main ) {
	int n;
	int count = 30;	  // default iteration count
	char ch = '2';	  // default character to print
	char buf[128];

	// process the command-line arguments
	switch( argc ) {
	case 3:	count = str2int( argv[2], 10 );
			// FALL THROUGH
	case 2:	ch = argv[1][0];
			break;
	default:
			sprint( buf, "main2: argc %d, args: ", argc );
			cwrites( buf );
			for( int i = 0; i <= argc; ++i ) {
				sprint( buf, " %s", argv[argc] ? argv[argc] : "(null)" );
				cwrites( buf );
			}
			cwrites( "\n" );
	}

	// announce our presence
	n = swritech( ch );
	if( n != 1 ) {
		sprint( buf, "== %c, write #1 returned %d\n", ch, n );
		cwrites( buf );
	}

	// iterate and print the required number of other characters
	for( int i = 0; i < count; ++i ) {
		DELAY(STD);
		n = swritech( ch );
		if( n != 1 ) {
			sprint( buf, "== %c, write #2 returned %d\n", ch, n );
			cwrites( buf );
		}
	}

	// all done!
	return( 0 );
}
