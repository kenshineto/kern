/**
** @file	kernel.c
**
** @author	CSCI-452 class of 20245
**
** @brief	Kernel support routines
*/

#define KERNEL_SRC

#include <common.h>
#include <cio.h>
#include <clock.h>
#include <kmem.h>
#include <procs.h>
#include <sio.h>
#include <syscalls.h>
#include <user.h>
#include <userids.h>
#include <vm.h>

/*
** PRIVATE DEFINITIONS
*/

/*
** PRIVATE DATA TYPES
*/

/*
** PRIVATE GLOBAL VARIABLES
*/

/*
** PUBLIC GLOBAL VARIABLES
*/

// character buffers, usable throughout the OS
// nto guaranteed to retain their contents across an exception return
char b256[256]; // primarily used for message creation
char b512[512]; // used by PANIC macro

/*
** PRIVATE FUNCTIONS
*/

/*
** PRIVATE FUNCTIONS
*/

/**
** report - report the system configuration
**
** Prints configuration information about the OS on the console monitor.
**
** @param dtrace  Decode the TRACE options
*/
static void kreport(bool_t dtrace)
{
	cio_puts("\n-------------------------------\n");
	cio_printf("Config:  N_PROCS = %d", N_PROCS);
	cio_printf(" N_PRIOS = %d", N_PRIOS);
	cio_printf(" N_STATES = %d", N_STATES);
	cio_printf(" CLOCK = %dHz\n", CLOCK_FREQ);

	// This code is ugly, but it's the simplest way to
	// print out the values of compile-time options
	// without spending a lot of execution time at it.

	cio_puts("Options: "
#ifdef RPT_INT_UNEXP
			 " R-uint"
#endif
#ifdef RPT_INT_MYSTERY
			 " R-mint"
#endif
#ifdef TRACE_CX
			 " CX"
#endif
#ifdef CONSOLE_STATS
			 " Cstats"
#endif
	); // end of cio_puts() call

#ifdef SANITY
	cio_printf(" SANITY = %d", SANITY);
#endif
#ifdef STATUS
	cio_printf(" STATUS = %d", STATUS);
#endif

#if TRACE > 0
	cio_printf(" TRACE = 0x%04x\n", TRACE);

	// decode the trace settings if that was requested
	if (TRACING_SOMETHING && dtrace) {
		// this one is simpler - we rely on string literal
		// concatenation in the C compiler to create one
		// long string to print out

		cio_puts("Tracing:"
#if TRACING_PCB
				 " PCB"
#endif
#if TRACING_VM
				 " VM"
#endif
#if TRACING_QUEUE
				 " QUE"
#endif
#if TRACING_SCHED
				 " SCHED"
#endif
#if TRACING_DISPATCH
				 " DISPATCH"
#endif
#if TRACING_SYSCALLS
				 " SCALL"
#endif
#if TRACING_SYSRETS
				 " SRET"
#endif
#if TRACING_EXIT
				 " EXIT"
#endif
#if TRACING_INIT
				 " INIT"
#endif
#if TRACING_KMEM
				 " KM"
#endif
#if TRACING_KMEM_FREELIST
				 " KMFL"
#endif
#if TRACING_KMEM_INIT
				 " KMIN"
#endif
#if TRACING_FORK
				 " FORK"
#endif
#if TRACING_EXEC
				 " EXEC"
#endif
#if TRACING_SIO_STAT
				 " S_STAT"
#endif
#if TRACING_SIO_ISR
				 " S_ISR"
#endif
#if TRACING_SIO_RD
				 " S_RD"
#endif
#if TRACING_SIO_WR
				 " S_WR"
#endif
#if TRACING_USER
				 " USER"
#endif
#if TRACING_ELF
				 " ELF"
#endif
		); // end of cio_puts() call
	}
#endif /* TRACE > 0 */

	cio_puts("\n-------------------------------\n");
}

#if defined(CONSOLE_STATS)
/**
** stats - callback routine for console statistics
**
** Called by the CIO module when a key is pressed on the
** console keyboard.  Depending on the key, it will print
** statistics on the console display, or will cause the
** user shell process to be dispatched.
**
** This code runs as part of the CIO ISR.
*/
static void stats(int code)
{
	switch (code) {
	case 'a': // dump the active table
		ptable_dump("\nActive processes", false);
		break;

	case 'c': // dump context info for all active PCBs
		ctx_dump_all("\nContext dump");
		break;

	case 'p': // dump the active table and all PCBs
		ptable_dump("\nActive processes", true);
		break;

	case 'q': // dump the queues
		// code to dump out any/all queues
		pcb_queue_dump("R", ready);
		pcb_queue_dump("W", waiting);
		pcb_queue_dump("S", sleeping);
		pcb_queue_dump("Z", zombie);
		pcb_queue_dump("I", sioread);
		break;

	case 'r': // print system configuration information
		report(true);
		break;

		// ignore CR and LF
	case '\r': // FALL THROUGH
	case '\n':
		break;

	default:
		cio_printf("console: unknown request '0x%02x'\n", code);
		// FALL THROUGH

	case 'h': // help message
		cio_puts("\nCommands:\n"
				 "	 a	-- dump the active table\n"
				 "	 c	-- dump contexts for active processes\n"
				 "	 h	-- this message\n"
				 "	 p	-- dump the active table and all PCBs\n"
				 "	 q	-- dump the queues\n"
				 "	 r	-- print system configuration\n");
		break;
	}
}
#endif

/*
** PUBLIC FUNCTIONS
*/

/**
** main - system initialization routine
**
** Called by the startup code immediately before returning into the
** first user process.
**
** Making this type 'int' keeps the compiler happy.
*/
int main(void)
{
	/*
	** BOILERPLATE CODE - taken from basic framework
	**
	** Initialize interrupt stuff.
	*/

	init_interrupts(); // IDT and PIC initialization

	/*
	** Console I/O system.
	**
	** Does not depend on the other kernel modules, so we can
	** initialize it before we initialize the kernel memory
	** and queue modules.
	*/

#if defined(CONSOLE_STATS)
	cio_init(stats);
#else
	cio_init(NULL); // no console callback routine
#endif

	cio_clearscreen(); // wipe out whatever is there

	/*
	** TERM-SPECIFIC CODE STARTS HERE
	*/

	/*
	** Initialize various OS modules
	**
	** Other modules (clock, SIO, syscall, etc.) are expected to
	** install their own ISRs in their initialization routines.
	*/

	cio_puts("System initialization starting.\n");
	cio_puts("-------------------------------\n");

	cio_puts("Modules:");

	// call the module initialization functions, being
	// careful to follow any module precedence requirements

	km_init(); // MUST BE FIRST
#if TRACING_KMEM || TRACING_KMEM_FREE
	delay(DELAY_2_SEC); // approximately
#endif

	// other module initialization calls here
	clk_init(); // clock
	pcb_init(); // process (PCBs, queues, scheduler)
#if TRACING_PCB
	delay(DELAY_2_SEC);
#endif
	sio_init(); // serial i/o
	sys_init(); // system call
#if TRACING_SYSCALLS || TRACING_SYSRETS
	delay(DELAY_2_SEC);
#endif
	vm_init(); // virtual memory
	user_init(); // user code handling

	cio_puts("\nModule initialization complete.\n");
	cio_puts("-------------------------------\n");

	// report our configuration options
	kreport(true);

	delay(DELAY_3_SEC);

	/*
	** Other tasks typically performed here:
	**
	**	Enabling any I/O devices (e.g., SIO xmit/rcv)
	*/

	/*
	** Create the initial user process
	**
	** This code is largely stolen from the fork() and exec()
	** implementations in syscalls.c; if those change, this must
	** also change.
	*/

	// if we can't get a PCB, there's no use continuing!
	assert(pcb_alloc(&init_pcb) == SUCCESS);

	// fill in the necessary details
	init_pcb->pid = PID_INIT;
	init_pcb->state = STATE_NEW;
	init_pcb->priority = PRIO_HIGH;

	// find the 'init' program
	prog_t *prog = user_locate(Init);
	assert(prog != NULL);

	// command-line arguments for 'init'
	const char *args[2] = { "init", NULL };

	// load it
	assert(user_load(prog, init_pcb, args) == SUCCESS);

	// send it on its merry way
	schedule(init_pcb);

#ifdef TRACE_CX
	// if we're using a scrolling region, wait a bit more and then set it up
	delay(DELAY_7_SEC);

	// define a scrolling region in the top 7 lines of the screen
	cio_setscroll(0, 7, 99, 99);

	// clear it
	cio_clearscroll();

	// clear the top line
	cio_puts_at(
		0, 0,
		"*                                                                               ");
	// separator
	cio_puts_at(
		0, 6,
		"================================================================================");
#endif

	// switch to the "real" kernel page directory
	vm_set_kvm();

	/*
	** END OF TERM-SPECIFIC CODE
	**
	** Finally, report that we're all done.
	*/

	cio_puts("System initialization complete.\n");
	cio_puts("-------------------------------\n");

	sio_enable(SIO_RX);

	return 0;
}
