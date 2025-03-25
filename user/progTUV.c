#include <common.h>

/**
** User function main #6:  exit, fork, exec, kill, waitpid, sleep, write
**
** Reports, then loops spawing userW, sleeps, then waits for or kills
** all its children.
**
** Invoked as:  main6  x  c  b
**	 where x is the ID character
**		   c is the child count
**		   b is wait/kill indicator ('w', 'W', or 'k')
*/

#ifndef MAX_CHILDREN
#define MAX_CHILDREN	50
#endif

USERMAIN( main ) {
	int count = 3;			// default child count
	char ch = '6';			// default character to print
	int nap = 8;			// nap time
	bool_t waiting = true;	// default is waiting by PID
	bool_t bypid = true;
	char buf[128];
	uint_t children[MAX_CHILDREN];
	int nkids = 0;
	char ch2[] = "*?*";

	// process the command-line arguments
	switch( argc ) {
	case 4:	waiting = argv[3][0] != 'k';	// 'w'/'W' -> wait, else -> kill
			bypid   = argv[3][0] != 'w';	// 'W'/'k' -> by PID
			// FALL THROUGH
	case 3:	count = str2int( argv[2], 10 );
			// FALL THROUGH
	case 2:	ch = argv[1][0];
			break;
	default:
			sprint( buf, "main6: argc %d, args: ", argc );
			cwrites( buf );
			for( int i = 0; i <= argc; ++i ) {
				sprint( buf, " %s", argv[argc] ? argv[argc] : "(null)" );
				cwrites( buf );
			}
			cwrites( "\n" );
	}

	// fix the secondary output message (for indicating errors)
	ch2[1] = ch;

	// announce our presence
	write( CHAN_SIO, &ch, 1 );

	// set up the argument vector
	char *argsw[] = { "userW", "W", "10", "5", NULL };

	for( int i = 0; i < count; ++i ) {
		int whom = spawn( ProgW, argsw );
		if( whom < 0 ) {
			swrites( ch2 );
		} else {
			children[nkids++] = whom;
		}
	}

	// let the children start
	sleep( SEC_TO_MS(nap) );

	// collect exit status information

	// current child index
	int n = 0;

	do {
		int this;
		int32_t status;

		// are we waiting for or killing it?
		if( waiting ) {
			this = waitpid( bypid ? children[n] : 0, &status );
		} else {
			// always by PID
			this = kill( children[n] );
		}

		// what was the result?
		if( this < SUCCESS ) {

			// uh-oh - something went wrong

			// "no children" means we're all done
			if( this != E_NO_CHILDREN ) {
				if( waiting ) {
					sprint( buf, "!! %c: waitpid(%d) status %d\n",
							ch, bypid ? children[n] : 0, this );
				} else {
					sprint( buf, "!! %c: kill(%d) status %d\n",
							ch, children[n], this );
				}
			} else {
				sprint( buf, "!! %c: no children\n", ch );
			}

			// regardless, we're outta here
			break;

		} else {

			// locate the child
			int ix = -1;

			// were we looking by PID?
			if( bypid ) {
				// we should have just gotten the one we were looking for
				if( this != children[n] ) {
					// uh-oh
					sprint( buf, "** %c: wait/kill PID %d, got %d\n",
							ch, children[n], this );
					cwrites( buf );
				} else {
					ix = n;
				}
			}

			// either not looking by PID, or the lookup failed somehow
			if( ix < 0 ) {
				int i;
				for( i = 0; i < nkids; ++i ) {
					if( children[i] == this ) {
						ix = i;
						break;
					}
				}
			}

			// if ix == -1, the PID we received isn't in our list of children

			if( ix < 0 ) {

				// didn't find an entry for this PID???
				sprint( buf, "!! %c: child PID %d term, NOT FOUND\n",
						ch, this );

			} else {

				// found this PID in our list of children
				if( ix != n ) {
					// ... but it's out of sequence
					sprint( buf, "== %c: child %d (%d,%d) status %d\n",
							ch, ix, n, this, status );
				} else {
					sprint( buf, "== %c: child %d (%d) status %d\n",
							ch, ix, this, status );
				}
			}

		}

		cwrites( buf );

		++n;

	} while( n < nkids );

	exit( 0 );

	return( 42 );  // shut the compiler up!
}
