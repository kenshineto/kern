#include <common.h>

/**
** User function H:  exit, fork, exec, sleep, write
**
** Prints its ID, then spawns 'n' children; exits before they terminate.
**
** Invoked as:  userH  x  n
**	 where x is the ID character
**		   n is the number of children to spawn
*/

USERMAIN( main ) {
	int32_t ret = 0;  // return value
	int count = 5;	  // child count
	char ch = 'h';	  // default character to print
	char buf[128];
	int whom;

	// process the argument(s)
	switch( argc ) {
	case 3:	count = str2int( argv[2], 10 );
			// FALL THROUGH
	case 2:	ch = argv[1][0];
			break;
	default:
			sprint( buf, "userH: argc %d, args: ", argc );
			cwrites( buf );
			for( int i = 0; i <= argc; ++i ) {
				sprint( buf, " %s", argv[argc] ? argv[argc] : "(null)" );
				cwrites( buf );
			}
			cwrites( "\n" );
	}

	// announce our presence
	swritech( ch );

	// we spawn user Z and then exit before it can terminate
	// userZ 'Z' 10

	char *argsz[] = { "userZ", "Z", "10", NULL };

	for( int i = 0; i < count; ++i ) {

		// spawn a child
		whom = spawn( ProgZ, argsz );

		// our exit status is the number of failed spawn() calls
		if( whom < 0 ) {
			sprint( buf, "!! %c spawn() failed, returned %d\n", ch, whom );
			cwrites( buf );
			ret += 1;
		}
	}

	// yield the CPU so that our child(ren) can run
	sleep( 0 );

	// announce our departure
	swritech( ch );

	exit( ret );

	return( 42 );  // shut the compiler up!
}
