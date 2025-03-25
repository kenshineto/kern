#include <common.h>

/**
** User function Q:   exit, write, bogus
**
** Reports itself, then tries to execute a bogus system call
**
** Invoked as:  userQ  x
**	 where x is the ID character
*/

USERMAIN( main ) {
	char ch = 'q';	  // default character to print
	char buf[128];

	// process the command-line arguments
	switch( argc ) {
	case 2:	ch = argv[1][0];
			break;
	default:
			sprint( buf, "userQ: argc %d, args: ", argc );
			cwrites( buf );
			for( int i = 0; i <= argc; ++i ) {
				sprint( buf, " %s", argv[argc] ? argv[argc] : "(null)" );
				cwrites( buf );
			}
			cwrites( "\n" );
	}

	// announce our presence
	write( CHAN_SIO, &ch, 1 );

	// try something weird
	bogus();

	// should not have come back here!
	sprint( buf, "!!!!! %c returned from bogus syscall!?!?!\n", ch );
	cwrites( buf );

	exit( 1 );

	return( 42 );  // shut the compiler up!
}
