#include <common.h>

/**
** Initial process; it starts the other top-level user processes.
**
** Prints a message at startup, '+' after each user process is spawned,
** and '!' before transitioning to wait() mode to the SIO, and
** startup and transition messages to the console. It also reports
** each child process it collects via wait() to the console along
** with that child's exit status.
*/

/*
** "Spawn table" process entry. Similar to the one in shell.c, but
** this version has a field to hold the PID of the spawned process
** to allow 'init' to respawn it when it terminates.
*/
typedef struct proc_s {
	uint_t index;           // process table index
	uint_t pid;             // its PID (when spawned)
	uint8_t e_prio;         // process priority
	char select[3];         // identifying character, NUL, extra
	char *args[MAX_ARGS];   // argument vector strings
} proc_t;

/*
** Create a spawn table entry for a process with a string literal
** as its argument buffer.	We rely on the fact that the C standard
** ensures our array of pointers will be filled out with NULLs
*/
#define PROCENT(e,p,s,...) { e, 0, p, s, { __VA_ARGS__ , NULL } }

// sentinel value for the end of the table - must be updated
// if you have more than 90,210 user programs in the table
#define	TBLEND	90210

/*
** This table contains one entry for each process that should be
** started by 'init'. Typically, this includes the 'idle' process
** and a 'shell' process.
*/
static proc_t spawn_table[] = {

	// the idle process; it runs at Deferred priority,
	// so it will only be dispatched when there is
	// nothing else available to be dispatched
	PROCENT( Idle, PRIO_DEFERRED, "!", "idle", "." ),

	// the user shell
	PROCENT( Shell, PRIO_STD, "@", "shell" ),

	// PROCENT( 0, 0, 0, 0 )
	{ TBLEND }
};

// character to be printed by init when it spawns a process
static char ch = '+';

/**
** process - spawn all user processes listed in the supplied table
**
** @param proc  pointer to the spawn table entry to be used
*/

static void process( proc_t *proc )
{
	char buf[128];

	// kick off the process
	int32_t p = fork();
	if( p < 0 ) {

		// error!
		sprint( buf, "INIT: fork for #%d failed\n",
				(uint32_t) (proc->index) );
		cwrites( buf );

	} else if( p == 0 ) {

		// change child's priority
		(void) setprio( proc->e_prio );

		// now, send it on its way
		exec( proc->index, proc->args );

		// uh-oh - should never get here!
		sprint( buf, "INIT: exec(0x%08x) failed\n",
				(uint32_t) (proc->index) );
		cwrites( buf );

	} else {

		// parent just reports that another one was started
		swritech( ch );

		proc->pid = p;

	}
}

/*
** The initial user process. Should be invoked with zero or one
** argument; if provided, the first argument should be the ASCII
** character 'init' will print to indicate the spawning of a process.
*/
USERMAIN( main ) {
	char buf[128];

	// check to see if we got a non-standard "spawn" character
	if( argc > 1 ) {
		// maybe - check it to be sure it's printable
		uint_t i = argv[1][0];
		if( i > ' ' && i < 0x7f ) {
			ch = argv[1][0];
		}
	}

	cwrites( "Init started\n" );

	// home up, clear on a TVI 925
	swritech( '\x1a' );

	// wait a bit
	DELAY(SHORT);

	// a bit of Dante to set the mood :-)
	swrites( "\n\nSpem relinquunt qui huc intrasti!\n\n\r" );

	/*
	** Start all the user processes
	*/

	cwrites( "INIT: starting user processes\n" );

	proc_t *next;
	for( next = spawn_table; next->index != TBLEND; ++next ) {
		process( next );
	}

	swrites( " !!!\r\n\n" );

	/*
	** At this point, we go into an infinite loop waiting
	** for our children (direct, or inherited) to exit.
	*/

	cwrites( "INIT: transitioning to wait() mode\n" );

	for(;;) {
		int32_t status;
		int whom = waitpid( 0, &status );

		// PIDs must be positive numbers!
		if( whom <= 0 ) {
			sprint( buf, "INIT: waitpid() returned %d???\n", whom );
			cwrites( buf );
		} else {

			// got one; report it
			sprint( buf, "INIT: pid %d exit(%d)\n", whom, status );
			cwrites( buf );

			// figure out if this is one of ours
			for( next = spawn_table; next->index != TBLEND; ++next ) {
				if( next->pid == whom ) {
					// one of ours - reset the PID field
					// (in case the spawn attempt fails)
					next->pid = 0;
					// and restart it
					process( next );
					break;
				}
			}
		}
	}

	/*
	** SHOULD NEVER REACH HERE
	*/

	cwrites( "*** INIT IS EXITING???\n" );
	exit( 1 );

	return( 1 );  // shut the compiler up
}
