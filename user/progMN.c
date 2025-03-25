#include <common.h>

/**
** User function main #5:  exit, fork, exec, write
**
** Iterates spawning copies of userW (and possibly userZ), reporting
** their PIDs as it goes.
**
** Invoked as:  main5  x  n  b
**	 where x is the ID character
**		   n is the iteration count
**		   b is the w&z boolean
*/

USERMAIN( main ) {
	int count = 5;	// default iteration count
	char ch = '5';	// default character to print
	int alsoZ = 0;	// also do userZ?
	char msgw[] = "*5w*";
	char msgz[] = "*5z*";
	char buf[128];

	// process the command-line arguments
	switch( argc ) {
	case 4:	alsoZ = argv[3][0] == 't';
			// FALL THROUGH
	case 3:	count = str2int( argv[2], 10 );
			// FALL THROUGH
	case 2:	ch = argv[1][0];
			break;
	default:
			sprint( buf, "main5: argc %d, args: ", argc );
			cwrites( buf );
			for( int i = 0; i <= argc; ++i ) {
				sprint( buf, " %s", argv[argc] ? argv[argc] : "(null)" );
				cwrites( buf );
			}
			cwrites( "\n" );
	}

	// update the extra message strings
	msgw[1] = msgz[1] = ch;

	// announce our presence
	write( CHAN_SIO, &ch, 1 );

	// set up the argument vector(s)

	// W:  15 iterations, 5-second sleep
	char *argsw[] = { "userW", "W", "15", "5", NULL };

	// Z:  15 iterations
	char *argsz[] = { "userZ", "Z", "15", NULL };

	for( int i = 0; i < count; ++i ) {
		write( CHAN_SIO, &ch, 1 );
		int whom = spawn( ProgW, argsw	);
		if( whom < 1 ) {
			swrites( msgw );
		}
		if( alsoZ ) {
			whom = spawn( ProgZ, argsz );
			if( whom < 1 ) {
				swrites( msgz );
			}
		}
	}

	exit( 0 );

	return( 42 );  // shut the compiler up!
}
