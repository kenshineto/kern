#include <common.h>

/**
** User function J:   exit, fork, exec, write
**
** Reports, tries to spawn lots of children, then exits
**
** Invoked as:  userJ  x  [ n ]
**	 where x is the ID character
**		   n is the number of children to spawn (defaults to 2 * N_PROCS)
*/

USERMAIN( main ) {
	int count = 2 * N_PROCS;	// number of children to spawn
	char ch = 'j';				// default character to print
	char buf[128];

	// process the command-line arguments
	switch( argc ) {
	case 3:	count = str2int( argv[2], 10 );
			// FALL THROUGH
	case 2:	ch = argv[1][0];
			break;
	default:
			sprint( buf, "userJ: argc %d, args: ", argc );
			cwrites( buf );
			for( int i = 0; i <= argc; ++i ) {
				sprint( buf, " %s", argv[argc] ? argv[argc] : "(null)" );
				cwrites( buf );
			}
			cwrites( "\n" );
	}

	// announce our presence
	write( CHAN_SIO, &ch, 1 );

	// set up the command-line arguments
	char *argsy[] = { "userY", "Y", "10", NULL };

	for( int i = 0; i < count ; ++i ) {
		int whom = spawn( ProgY, argsy );
		if( whom < 0 ) {
			write( CHAN_SIO, "!j!", 3 );
		} else {
			write( CHAN_SIO, &ch, 1 );
		}
	}

	exit( 0 );

	return( 42 );  // shut the compiler up!
}
