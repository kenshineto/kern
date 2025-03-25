#include <common.h>

// should we keep going?
static bool_t time_to_stop = false;

// number of spawned but uncollected children
static int children = 0;

/*
** For the test programs in the baseline system, command-line arguments
** follow these rules. The first two entries are as follows:
**
**	argv[0] the name used to "invoke" this process
**	argv[1] the "character to print" (identifies the process)
**
** Most user programs have one or more additional arguments.
**
** See the comment at the beginning of each user-code source file for
** information on the argument list that code expects.
*/

/*
** "Spawn table" process entry. Similar to that in init.c,
** except this one has no place to store the PID of the child.
*/
typedef struct proc_s {
	uint_t index;           // process table index
	int8_t prio;            // process priority
	char select[3];         // identifying character, NUL, extra
	char *args[MAX_ARGS];   // argument vector strings
} proc_t;

/*
** Create a spawn table entry for a process with a string literal
** as its argument buffer.	We rely on the fact that the C standard
** ensures our array of pointers will be filled out with NULLs
*/
#define PROCENT(e, p, s, ...) { e, p, s, { __VA_ARGS__ , NULL } }

// sentinel value for the end of the table - must be updated
// if you have more than 90,210 user programs in the table
#define	TBLEND	90210

/*
** The spawn table contains entries for processes that are started
** by the shell.
*/
static proc_t spawn_table[] = {

	// Users A-C each run ProgABC, which loops printing its character
#if defined(SPAWN_A)
	PROCENT( ProgABC, PRIO_STD, "A", "userA", "A", "30" ),
#endif
#if defined(SPAWN_B)
	PROCENT( ProgABC, PRIO_STD, "B", "userB", "B", "30" ),
#endif
#if defined(SPAWN_C)
	PROCENT( ProgABC, PRIO_STD, "C", "userC", "C", "30" ),
#endif

	// Users D and E run ProgDE, which is like ProgABC but doesn't exit()
#if defined(SPAWN_D)
	PROCENT( ProgDE, PRIO_STD, "D", "userD", "D", "20" ),
#endif
#if defined(SPAWN_E)
	PROCENT( ProgDE, PRIO_STD, "E", "userE", "E", "20" ),
#endif

	// Users F and G run ProgFG, which sleeps between write() calls
#if defined(SPAWN_F)
	PROCENT( ProgFG, PRIO_STD, "F", "userF", "F", "20" ),
#endif
#if defined(SPAWN_G)
	PROCENT( ProgFG, PRIO_STD, "G", "userG", "G", "10" ),
#endif

	// User H tests reparenting of orphaned children
#if defined(SPAWN_H)
	PROCENT( ProgH, PRIO_STD, "H", "userH", "H", "4" ),
#endif

	// User I spawns several children, kills one, and waits for all
#if defined(SPAWN_I)
	PROCENT( ProgI, PRIO_STD, "I", "userI", "I" ),
#endif

	// User J tries to spawn 2 * N_PROCS children
#if defined(SPAWN_J)
	PROCENT( ProgJ, PRIO_STD, "J", "userJ", "J" ),
#endif

	// Users K and L iterate spawning userX and sleeping
#if defined(SPAWN_K)
	PROCENT( ProgKL, PRIO_STD, "K", "userK", "K", "8" ),
#endif
#if defined(SPAWN_L)
	PROCENT( ProgKL, PRIO_STD, "L", "userL", "L", "5" ),
#endif

	// Users M and N spawn copies of userW and userZ via ProgMN
#if defined(SPAWN_M)
	PROCENT( ProgMN, PRIO_STD, "M", "userM", "M", "5", "f" ),
#endif
#if defined(SPAWN_N)
	PROCENT( ProgMN, PRIO_STD, "N", "userN", "N", "5", "t" ),
#endif

	// There is no user O

	// User P iterates, reporting system time and stats, and sleeping
#if defined(SPAWN_P)
	PROCENT( ProgP, PRIO_STD, "P", "userP", "P", "3", "2" ),
#endif

	// User Q tries to execute a bad system call
#if defined(SPAWN_Q)
	PROCENT( ProgQ, PRIO_STD, "Q", "userQ", "Q" ),
#endif

	// User R reports its PID, PPID, and sequence number; it
	// calls fork() but not exec(), with each child getting the
	// next sequence number, to a total of five copies
#if defined(SPAWN_R)
	PROCENT( ProgR, PRIO_STD, "R", "userR", "R", "20", "1" ),
#endif

	// User S loops forever, sleeping 13 sec. on each iteration
#if defined(SPAWN_S)
	PROCENT( ProgS, PRIO_STD, "S", "userS", "S", "13" ),
#endif

	// Users T-V run ProgTUV(); they spawn copies of userW
	//	 User T waits for any child
	//	 User U waits for each child by PID
	//	 User V kills each child
#if defined(SPAWN_T)
	PROCENT( ProgTUV, PRIO_STD, "T", "userT", "T", "6", "w" ),
#endif
#if defined(SPAWN_U)
	PROCENT( ProgTUV, PRIO_STD, "U", "userU", "U", "6", "W" ),
#endif
#if defined(SPAWN_V)
	PROCENT( ProgTUV, PRIO_STD, "V", "userV", "V", "6", "k" ),
#endif
	
	// a dummy entry to use as a sentinel
	{ TBLEND }

	// these processes are spawned by the ones above, and are never
	// spawned directly.

	// PROCENT( ProgW, PRIO_STD, "?", "userW", "W", "20", "3" ),
	// PROCENT( ProgX, PRIO_STD, "?", "userX", "X", "20" ),
	// PROCENT( ProgY, PRIO_STD, "?", "userY", "Y", "10" ),
	// PROCENT( ProgZ, PRIO_STD, "?", "userZ", "Z", "10" )
};

/*
** usage function
*/
static void usage( void ) {
	swrites( "\nTests - run with '@x', where 'x' is one or more of:\n " );
	proc_t *p = spawn_table;
	while( p->index != TBLEND ) {
		swritech( ' ' );
		swritech( p->select[0] );
	}
	swrites( "\nOther commands: @* (all), @h (help), @x (exit)\n" );
}

/*
** run a program from the program table, or a builtin command
*/
static int run( char which ) {
	char buf[128];
	register proc_t *p;

	if( which == 'h' ) {

		// builtin "help" command
		usage();

	} else if( which == 'x' ) {

		// builtin "exit" command
		time_to_stop = true;

	} else if( which == '*' ) {

		// torture test! run everything!
		for( p = spawn_table; p->index != TBLEND; ++p ) {
			int status = spawn( p->index, p->args );
			if( status > 0 ) {
				++children;
			}
		}

	} else {

		// must be a single test; find and run it
		for( p = spawn_table; p->index != TBLEND; ++p ) {
			if( p->select[0] == which ) {
				// found it!
				int status = spawn( p->index, p->args );
				if( status > 0 ) {
					++children;
				}
				return status;
			}
		}

		// uh-oh, made it through the table without finding the program
		sprint( buf, "shell: unknown cmd '%c'\n", which );
		swrites( buf );
		usage();
	}

	return 0;
}

/**
** edit - perform any command-line editing we need to do
**
** @param line   Input line buffer
** @param n      Number of valid bytes in the buffer
*/
static int edit( char line[], int n ) {
	char *ptr = line + n - 1;	// last char in buffer

	// strip the EOLN sequence
	while( n > 0 ) {
		if( *ptr == '\n' || *ptr == '\r' ) {
			--n;
		} else {
			break;
		}
	}

	// add a trailing NUL byte
	if( n > 0 ) {
		line[n] = '\0';
	}

	return n;
}

/**
** shell - extremely simple shell for spawning test programs
**
** Scheduled by _kshell() when the character 'u' is typed on
** the console keyboard.
*/
USERMAIN( main ) {

	// keep the compiler happy
	(void) argc;
	(void) argv;

	// report that we're up and running
	swrites( "Shell is ready\n" );

	// print a summary of the commands we'll accept
	usage();

	// loop forever
	while( !time_to_stop ) {
		char line[128];
		char *ptr;

		// the shell reads one line from the keyboard, parses it,
		// and performs whatever command it requests.

		swrites( "\n> " );
		int n = read( CHAN_SIO, line, sizeof(line) );
		
		// shortest valid command is "@?", so must have 3+ chars here
		if( n < 3 ) {
			// ignore it
			continue;
		}

		// edit it as needed; new shortest command is 2+ chars
		if( (n=edit(line,n)) < 2 ) {
			continue;
		}

		// find the '@'
		int i = 0;
		for( ptr = line; i < n; ++i, ++ptr ) {
			if( *ptr == '@' ) {
				break;
			}
		}

		// did we find an '@'?
		if( i < n ) {

			// yes; process any commands that follow it
			++ptr;

			for( ; *ptr != '\0'; ++ptr ) {
				char buf[128];
				int pid = run( *ptr );

				if( pid < 0 ) {
					// spawn() failed
					sprint( buf, "+++ Shell spawn %c failed, code %d\n",
							*ptr, pid );
					cwrites( buf );
				}

				// should we end it all?
				if( time_to_stop ) {
					break;
				}
			} // for

			// now, wait for all the spawned children
			while( children > 0 ) {
				// wait for the child
				int32_t status;
				char buf[128];
				int whom = waitpid( 0, &status );

				// figure out the result
				if( whom == E_NO_CHILDREN ) {
					break;
				} else if( whom < 1 ) {
					sprint( buf, "shell: waitpid() returned %d\n", whom );
				} else {
					--children;
					sprint( buf, "shell: PID %d exit status %d\n",
							whom, status );
				}
				// report it
				swrites( buf );
			}
		}  // if i < n
	}  // while

	cwrites( "!!! shell exited loop???\n" );
	exit( 1 );
}
