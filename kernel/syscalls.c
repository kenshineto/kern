/**
** @file	syscalls.c
**
** @author	CSCI-452 class of 20245
**
** @brief	System call implementations
*/

#define	KERNEL_SRC

#include <common.h>

#include <cio.h>
#include <clock.h>
#include <procs.h>
#include <sio.h>
#include <syscalls.h>
#include <user.h>
#include <vm.h>
#include <x86/pic.h>

/*
** PRIVATE DEFINITIONS
*/

/*
** Macros to simplify tracing a bit
**
** TRACING_SYSCALLS and TRACING_SYSRETS are defined in debug.h,
** controlled by the TRACE ** macro. If not tracing these, SYSCALL_ENTER
** is a no-op, and SYSCALL_EXIT just does a return.
*/

#if TRACING_SYSCALLS

#define SYSCALL_ENTER(x)    do { \
		cio_printf( "--> %s, pid %08x", __func__, (uint32_t) (x) ); \
	} while(0)

#else

#define SYSCALL_ENTER(x)    /* */

#endif  /* TRACING_SYSCALLS */

#if TRACING_SYSRETS

#define SYSCALL_EXIT(x) do { \
		cio_printf( "<-- %s %08x\n", __func__, (uint32_t) (x) ); \
		return; \
	} while(0)

#else

#define SYSCALL_EXIT(x) return

#endif  /* TRACING_SYSRETS */

/*
** PRIVATE DATA TYPES
*/

/*
** PUBLIC GLOBAL VARIABLES
*/

/*
** IMPLEMENTATION FUNCTIONS
*/

// a macro to simplify syscall entry point specification
// we don't declare these static because we may want to call
// some of them from other parts of the kernel
#define SYSIMPL(x)	void sys_##x( pcb_t * pcb )

/*
** Second-level syscall handlers
**
** All have this prototype:
**
**	static void sys_NAME( pcb_t *pcb );
**
** where the parameter 'pcb' is a pointer to the PCB of the process
** making the system call.
**
** Values being returned to the user are placed into the EAX
** field in the context save area for that process.
*/

/**
** sys_exit - terminate the calling process
**
** Implements:
**		void exit( int32_t status );
**
** Does not return
*/
SYSIMPL(exit) {

	// sanity check
	assert( pcb != NULL );

	SYSCALL_ENTER( pcb->pid );

	// retrieve the exit status of this process
	pcb->exit_status = (int32_t) ARG(pcb,1);

	// now, we need to do the following:
	// 	reparent any children of this process and wake up init if need be
	// 	find this process' parent and wake it up if it's waiting
	
	pcb_zombify( pcb );

	// pick a new winner
	dispatch();

	SYSCALL_EXIT( 0 );
}

/**
** sys_waitpid - wait for a child process to terminate
**
** Implements:
**		int waitpid( uint_t pid, int32_t *status );
**
** Blocks the calling process until the specified child (or any child)
** of the caller terminates. Intrinsic return is the PID of the child that
** terminated, or an error code; on success, returns the child's termination
** status via 'status' if that pointer is non-NULL.
*/
SYSIMPL(waitpid) {

	// sanity check
	assert( pcb != NULL );

	SYSCALL_ENTER( pcb->pid );

	/*
	** We need to do two things here:  (1) find out whether or
	** not this process has any children in the system, and (2)
	** find out whether the desired child (or any child, if the
	** target PID is 0) has terminated.
	**
	** To do this, we loop until we find a the requested PID or
	** a Zombie child process, or have gone through all of the
	** slots in the process table.
	**
	** If the target PID is 0, we don't care which child process
	** we reap here; there could be several, but we only need to
	** find one.
	*/

	// verify that we aren't looking for ourselves!
	uint_t target = ARG(pcb,1);

	if( target == pcb->pid ) {
		RET(pcb) = E_BAD_PARAM;
		SYSCALL_EXIT( E_BAD_PARAM );
	}

	// Good.  Now, figure out what we're looking for.

	pcb_t *child = NULL;

	if( target != 0 ) {

		// we're looking for a specific child
		child = pcb_find_pid( target );

		if( child != NULL ) {

			// found the process; is it one of our children:
			if( child->parent != pcb ) {
				// NO, so we can't wait for it
				RET(pcb) = E_BAD_PARAM;
				SYSCALL_EXIT( E_BAD_PARAM );
			}

			// yes!  is this one ready to be collected?
			if( child->state != STATE_ZOMBIE ) {
				// no, so we'll have to block for now
				child = NULL;
			}

		} else {

			// no such child
			RET(pcb) = E_BAD_PARAM;
			SYSCALL_EXIT( E_BAD_PARAM );

		}

	} else {

		// looking for any child

		// we need to find a process that is our child
		// and has already exited

		child = NULL;
		bool_t found = false;

		// unfortunately, we can't stop at the first child,
		// so we need to do the iteration ourselves
		register pcb_t *curr = ptable;

		for( int i = 0; i < N_PROCS; ++i, ++curr ) {

			if( curr->parent == pcb ) {

				// found one!
				found = true;

				// has it already exited?
				if( curr->state == STATE_ZOMBIE ) {
					// yes, so we're done here
					child = curr;
					break;
				}
			}
		}

		if( !found ) {
			// got through the loop without finding a child!
			RET(pcb) = E_NO_CHILDREN;
			SYSCALL_EXIT( E_NO_CHILDREN );
		}

	}

	/*
	** At this point, one of these situations is true:
	**
	**  * we are looking for a specific child and found it
	**  * we are looking for any child and found one
	**
	** Either way, 'child' will be non-NULL if the selected
	** process has already become a Zombie.  If that's the
	** case, we collect its status and clean it up; otherwise,
	** we block this process.
	*/

	// did we find one to collect?
	if( child == NULL ) {

		// no - mark the parent as "Waiting"
		pcb->state = STATE_WAITING;
		assert( pcb_queue_insert(waiting,pcb) == SUCCESS );

		// select a new current process
		dispatch();
		SYSCALL_EXIT( (uint32_t) current );
	}

	// found a Zombie; collect its information and clean it up
	RET(pcb) = child->pid;

	// get "status" pointer from parent
	int32_t *stat = (int32_t *) ARG(pcb,2);

	// if stat is NULL, the parent doesn't want the status
	if( stat != NULL ) {
		// ********************************************************
		// ** Potential VM issue here!  This code assigns the exit
		// ** status into a variable in the parent's address space.
		// ** This works in the baseline because we aren't using
		// ** any type of memory protection.  If address space
		// ** separation is implemented, this code will very likely
		// ** STOP WORKING, and will need to be fixed.
		// ********************************************************
		*stat = child->exit_status;
	}

	// clean up the child
	pcb_cleanup( child );

	SYSCALL_EXIT( RET(pcb) );
}

/**
** sys_fork - create a new process
**
** Implements:
**		int fork( void );
**
** Creates a new process that is a duplicate of the calling process.
** Returns the child's PID to the parent, and 0 to the child, on success;
** else, returns an error code to the parent.
*/
SYSIMPL(fork) {

	// sanity check
	assert( pcb != NULL );

	SYSCALL_ENTER( pcb->pid );

	// Make sure there's room for another process!
	pcb_t *new;
	if( pcb_alloc(&new) != SUCCESS || new == NULL ) {
		RET(pcb) = E_NO_PROCS;
		SYSCALL_EXIT( RET(pcb) );
	}

	// duplicate the memory image of the parent
	int status = user_duplicate( new, pcb );
	if( status != SUCCESS ) {
		pcb_free( new );
		RET(pcb) = status;
		SYSCALL_EXIT( status );
	}

	// Set the child's identity.
	new->pid = next_pid++;
	new->parent = pcb;
	new->state = STATE_NEW;

	// replicate other things inherited from the parent
	new->priority = pcb->priority;

	// Set the return values for the two processes.
	RET(pcb) = new->pid;
	RET(new) = 0;

	// Schedule the child, and let the parent continue.
	schedule( new );

	SYSCALL_EXIT( new->pid );
}

/**
** sys_exec - replace the memory image of a process
**
** Implements:
**		void exec( uint_t what, char **args );
**
** Replaces the memory image of the calling process with that of the
** indicated program.
**
** Returns only on failure.
*/
SYSIMPL(exec)
{
	// sanity check
	assert( pcb != NULL );

	uint_t what = ARG(pcb,1);
	const char **args = (const char **) ARG(pcb,2);

	SYSCALL_ENTER( pcb->pid );

	// locate the requested program
	prog_t *prog = user_locate( what );
	if( prog == NULL ) {
		RET(pcb) = E_NOT_FOUND;
		SYSCALL_EXIT( E_NOT_FOUND );
	}

	// we have located the program, but before we can load it,
	// we need to clean up the existing VM hierarchy
	vm_free( pcb->pdir );
	pcb->pdir = NULL;

	// "load" it and set up the VM tables for this process
	int status = user_load( prog, pcb, args );
	if( status != SUCCESS ) {
		RET(pcb) = status;
		SYSCALL_EXIT( status );
	}

	/*
	** Decision:
	**	(A) schedule this process and dispatch another,
	**	(B) let this one continue in its current time slice
	**	(C) reset this one's time slice and let it continue
	**
	** We choose option A.
	**
	** If scheduling the process fails, the exec() has failed. However,
	** all trace of the old process is gone by now, so we can't return
	** an error status to it.
	*/

	schedule( pcb );

	dispatch();
}

/**
** sys_read - read into a buffer from an input channel
**
** Implements:
**		int read( uint_t chan, void *buffer, uint_t length );
**
** Reads up to 'length' bytes from 'chan' into 'buffer'. Returns the
** count of bytes actually transferred.
*/
SYSIMPL(read) {

	// sanity check
	assert( pcb != NULL );

	SYSCALL_ENTER( pcb->pid );
	
	// grab the arguments
	uint_t chan = ARG(pcb,1);
	char *buf = (char *) ARG(pcb,2);
	uint_t len = ARG(pcb,3);

	// if the buffer is of length 0, we're done!
	if( len == 0 ) {
		RET(pcb) = 0;
		SYSCALL_EXIT( 0 );
	}

	// try to get the next character(s)
	int n = 0;

	if( chan == CHAN_CIO ) {

		// console input is non-blocking
		if( cio_input_queue() < 1 ) {
			RET(pcb) = 0;
			SYSCALL_EXIT( 0 );
		}
		// at least one character
		n = cio_gets( buf, len );
		RET(pcb) = n;
		SYSCALL_EXIT( n );

	} else if( chan == CHAN_SIO ) {

		// SIO input is blocking, so if there are no characters
		// available, we'll block this process
		n = sio_read( buf, len );
		RET(pcb) = n;
		SYSCALL_EXIT( n );

	}

	// bad channel code
	RET(pcb) = E_BAD_PARAM;
	SYSCALL_EXIT( E_BAD_PARAM );
}

/**
** sys_write - write from a buffer to an output channel
**
** Implements:
**		int write( uint_t chan, const void *buffer, uint_t length );
**
** Writes 'length' bytes from 'buffer' to 'chan'. Returns the
** count of bytes actually transferred.
*/
SYSIMPL(write) {

	// sanity check
	assert( pcb != NULL );

	SYSCALL_ENTER( pcb->pid );

	// grab the parameters
	uint_t chan = ARG(pcb,1);
	char *buf = (char *) ARG(pcb,2);
	uint_t length = ARG(pcb,3);

	// this is almost insanely simple, but it does separate the
	// low-level device access fromm the higher-level syscall implementation

	// assume we write the indicated amount
	int rval = length;

	// simplest case
	if( length >= 0 ) {

		if( chan == CHAN_CIO ) {

			cio_write( buf, length );

		} else if( chan == CHAN_SIO ) {

			sio_write( buf, length );

		} else {

			rval = E_BAD_CHAN;

		}

	}

	RET(pcb) = rval;

	SYSCALL_EXIT( rval );
}

/**
** sys_getpid - returns the PID of the calling process
**
** Implements:
**		uint_t getpid( void );
*/
SYSIMPL(getpid) {

	// sanity check!
	assert( pcb != NULL );

	SYSCALL_ENTER( pcb->pid );

	// return the time
	RET(pcb) = pcb->pid;
}

/**
** sys_getppid - returns the PID of the parent of the calling process
**
** Implements:
**		uint_t getppid( void );
*/
SYSIMPL(getppid) {

	// sanity check!
	assert( pcb != NULL );
	assert( pcb->parent != NULL );

	SYSCALL_ENTER( pcb->pid );

	// return the time
	RET(pcb) = pcb->parent->pid;
}

/**
** sys_gettime - returns the current system time
**
** Implements:
**		uint32_t gettime( void );
*/
SYSIMPL(gettime) {

	// sanity check!
	assert( pcb != NULL );

	SYSCALL_ENTER( pcb->pid );

	// return the time
	RET(pcb) = system_time;
}

/**
** sys_getprio - the scheduling priority of the calling process
**
** Implements:
**		int getprio( void );
*/
SYSIMPL(getprio) {

	// sanity check!
	assert( pcb != NULL );

	SYSCALL_ENTER( pcb->pid );

	// return the time
	RET(pcb) = pcb->priority;
}

/**
** sys_setprio - sets the scheduling priority of the calling process
**
** Implements:
**		int setprio( int new );
*/
SYSIMPL(setprio) {

	// sanity check!
	assert( pcb != NULL );

	SYSCALL_ENTER( pcb->pid );

	// remember the old priority
	int old = pcb->priority;

	// set the priority
	pcb->priority = ARG(pcb,1);

	// return the old value
	RET(pcb) = old;
}

/**
** sys_kill - terminate a process with extreme prejudice
**
** Implements:
**		int32_t kill( uint_t pid );
**
** Marks the specified process (or the calling process, if PID is 0)
** as "killed". Returns 0 on success, else an error code.
*/
SYSIMPL(kill) {

	// sanity check
	assert( pcb != NULL );

	SYSCALL_ENTER( pcb->pid );

	// who is the victim?
	uint_t pid = ARG(pcb,1);

	// if it's this process, convert this into a call to exit()
	if( pid == pcb->pid ) {
		pcb->exit_status = EXIT_KILLED;
		pcb_zombify( pcb );
		dispatch();
		SYSCALL_EXIT( EXIT_KILLED );
	}

	// must be a valid "ordinary user" PID
	// QUESTION: what if it's the idle process?
	if( pid < FIRST_USER_PID ) {
		RET(pcb) = E_FAILURE;
		SYSCALL_EXIT( E_FAILURE );
	}

	// OK, this is an acceptable victim; see if it exists
	pcb_t *victim = pcb_find_pid( pid );
	if( victim == NULL ) {
		// nope!
		RET(pcb) = E_NOT_FOUND;
		SYSCALL_EXIT( E_NOT_FOUND );
	}

	// must have a state that is possible
	assert( victim->state >= FIRST_VIABLE && victim->state < N_STATES );

	// how we perform the kill depends on the victim's state
	int32_t status = SUCCESS;

	switch( victim->state ) {

	case STATE_KILLED:    // FALL THROUGH
	case STATE_ZOMBIE:
		// you can't kill it if it's already dead
		RET(pcb) = SUCCESS;
		break;

	case STATE_READY:     // FALL THROUGH
	case STATE_SLEEPING:  // FALL THROUGH
	case STATE_BLOCKED:   // FALL THROUGH
		// here, the process is on a queue somewhere; mark
		// it as "killed", and let the scheduler deal with it
		victim->state = STATE_KILLED;
		RET(pcb) = SUCCESS;
		break;

	case STATE_RUNNING:
		// we have met the enemy, and it is us!
		pcb->exit_status = EXIT_KILLED;
		pcb_zombify( pcb );
		status = EXIT_KILLED;
		// we need a new current process
		dispatch();
		break;

	case STATE_WAITING:
		// similar to the 'running' state, but we don't need
		// to dispatch a new process
		victim->exit_status = EXIT_KILLED;
		status = pcb_queue_remove_this( waiting, victim );
		pcb_zombify( victim );
		RET(pcb) = status;
		break;

	default:
		// this is a really bad potential problem - we have an
		// unexpected or bogus process state, but we didn't
		// catch that earlier.
		sprint( b256, "*** kill(): victim %d, odd state %d\n",
				victim->pid, victim->state );
		PANIC( 0, b256 );
	}

	SYSCALL_EXIT( status );
}


/**
** sys_sleep - put the calling process to sleep for some length of time
**
** Implements:
**		uint_t sleep( uint_t ms );
**
** Puts the calling process to sleep for 'ms' milliseconds (or just yields
** the CPU if 'ms' is 0).  ** Returns the time the process spent sleeping.
*/
SYSIMPL(sleep) {

	// sanity check
	assert( pcb != NULL );

	SYSCALL_ENTER( pcb->pid );

	// get the desired duration
	uint_t length = ARG( pcb, 1 );

	if( length == 0 ) {

		// just yield the CPU
		// sleep duration is 0
		RET(pcb) = 0;

		// back on the ready queue
		schedule( pcb );

	} else {

		// sleep for a while
		pcb->wakeup = system_time + length;

		if( pcb_queue_insert(sleeping,pcb) != SUCCESS ) {
			// something strange is happening
			WARNING( "sleep pcb insert failed" );
			// if this is the current process, report an error
			if( current == pcb ) {
				RET(pcb) = -1;
			}
			// return without dispatching a new process
			return;
		}
	}

	// only dispatch if the current process called us
	if( pcb == current ) {
		current = NULL;
		dispatch();
	}
}

/*
** PRIVATE FUNCTIONS GLOBAL VARIABLES
*/

/*
** The system call jump table
**
** Initialized using designated initializers to ensure the entries
** are correct even if the syscall code values should happen to change.
** This also makes it easy to add new system call entries, as their
** position in the initialization list is irrelevant.
*/

static void (* const syscalls[N_SYSCALLS])( pcb_t * ) = {
	[ SYS_exit ]    = sys_exit,
	[ SYS_waitpid ] = sys_waitpid,
	[ SYS_fork ]    = sys_fork,
	[ SYS_exec ]    = sys_exec,
	[ SYS_read ]    = sys_read,
	[ SYS_write ]   = sys_write,
	[ SYS_getpid ]  = sys_getpid,
	[ SYS_getppid ] = sys_getppid,
	[ SYS_gettime ] = sys_gettime,
	[ SYS_getprio ] = sys_getprio,
	[ SYS_setprio ] = sys_setprio,
	[ SYS_kill ]    = sys_kill,
	[ SYS_sleep ]   = sys_sleep
};

/**
** Name:	sys_isr
**
** System call ISR
**
** @param vector   Vector number for this interrupt
** @param code     Error code (0 for this interrupt)
*/
static void sys_isr( int vector, int code ) {

	// keep the compiler happy
	(void) vector;
	(void) code;

	// sanity check!
	assert( current != NULL );
	assert( current->context != NULL );

	// retrieve the syscall code
	int num = REG( current, eax );

#if TRACING_SYSCALLS
	cio_printf( "** --> SYS pid %u code %u\n", current->pid, num );
#endif

	// validate it
	if( num < 0 || num >= N_SYSCALLS ) {
		// bad syscall number
		// could kill it, but we'll just force it to exit
		num = SYS_exit;
		ARG(current,1) = EXIT_BAD_SYSCALL;
	}

	// call the handler
	syscalls[num]( current );

#if TRACING_SYSCALLS
	cio_printf( "** <-- SYS pid %u ret %u\n", current->pid, RET(current) );
#endif

	// tell the PIC we're done
	outb( PIC1_CMD, PIC_EOI );
}

/*
** PUBLIC FUNCTIONS
*/

/**
** Name:  sys_init
**
** Syscall module initialization routine
**
** Dependencies:
**    Must be called after cio_init()
*/
void sys_init( void ) {

#if TRACING_INIT
	cio_puts( " Sys" );
#endif

	// install the second-stage ISR
	install_isr( VEC_SYSCALL, sys_isr );
}
