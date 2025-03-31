/*
** @file	procs.h
**
** @author	CSCI-452 class of 20245
**
** @brief	Process-related declarations
*/

#ifndef PROCS_H_
#define PROCS_H_

#include <common.h>

/*
** General (C and/or assembly) definitions
*/

#ifndef ASM_SRC

/*
** Start of C-only definitions
*/

/*
** Types
*/

/*
** Process states
*/
enum state_e {
	// pre-viable
	STATE_UNUSED = 0,
	STATE_NEW,
	// runnable
	STATE_READY,
	STATE_RUNNING,
	// runnable, but waiting for some event
	STATE_SLEEPING,
	STATE_BLOCKED,
	STATE_WAITING,
	// no longer runnable
	STATE_KILLED,
	STATE_ZOMBIE
	// sentinel value
	,
	N_STATES
};

// these may be handy for checking general conditions of processes
// they depend on the order of the state names in the enum!
#define FIRST_VIABLE STATE_READY
#define FIRST_BLOCKED STATE_SLEEPING
#define LAST_VIABLE STATE_WAITING

/*
** Process priorities are defined in <defs.h>
*/

/*
** Quantum lengths - values are number of clock ticks
*/
enum quantum_e { QUANTUM_SHORT = 1, QUANTUM_STANDARD = 3, QUANTUM_LONG = 5 };

/*
** PID-related definitions
*/
#define PID_INIT 1
#define FIRST_USER_PID 2

/*
** Process context structure
**
** NOTE:  the order of data members here depends on the
** register save code in isr_stubs.S!!!!
**
** This will be at the top of the user stack when we enter
** an ISR.  In the case of a system call, it will be followed
** by the return address and the system call parameters.
*/

typedef struct context_s {
	uint32_t ss; // pushed by isr_save
	uint32_t gs;
	uint32_t fs;
	uint32_t es;
	uint32_t ds;
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
	uint32_t vector;
	uint32_t code; // pushed by isr_save or the hardware
	uint32_t eip; // pushed by the hardware
	uint32_t cs;
	uint32_t eflags;
} context_t;

#define SZ_CONTEXT sizeof(context_t)

/*
** program section information for user processes
*/

typedef struct section_s {
	uint_t length; // length, in some units
	uint_t addr; // location, in some units
} section_t;

// note: these correspond to the PT_LOAD sections found in
// an ELF file, not necessarily to text/data/bss
#define SECT_L1 0
#define SECT_L2 1
#define SECT_L3 2
#define SECT_STACK 3

// total number of section table entries in our PCB
#define N_SECTS 4
// number of those that can be loaded from an ELF module
#define N_LOADABLE 3

/*
** The process control block
**
** Fields are ordered by size to avoid padding
**
** Currently, this is 72 bytes long. It could be reduced to 64 (2^6)
** bytes by making the last four fields uint16_t types; that would
** divide nicely into 1024 bytes, giving 16 PCBs per 1/4 page of memory.
*/

typedef struct pcb_s {
	// four-byte fields
	// start with these four bytes, for easy access in assembly
	context_t *context; // pointer to context save area on stack

	// VM information
	pde_t *pdir; // page directory for this process
	section_t sects[N_SECTS]; // per-section memory information

	// queue linkage
	struct pcb_s *next; // next PCB in queue

	// process state information
	struct pcb_s *parent; // pointer to PCB of our parent process
	uint32_t wakeup; // wakeup time, for sleeping processes
	int32_t exit_status; // termination status, for parent's use

	// these things may not need to be four bytes
	uint_t pid; // PID of this process
	enum state_e state; // process' current state
	enum priority_e priority; // process priority level
	uint_t ticks; // remaining ticks in this time slice

} pcb_t;

#define SZ_PCB sizeof(pcb_t)

/*
** PCB queue structure (opaque to the rest of the kernel)
*/
typedef struct pcb_queue_s *pcb_queue_t;

/*
** Queue ordering methods
*/
enum pcb_queue_order_e {
	O_FIFO,
	O_PRIO,
	O_PID,
	O_WAKEUP
	// sentinel
	,
	N_ORDERINGS
};
#define O_FIRST_STYLE O_FIFO
#define O_LAST_STYLE O_WAKEUP

/*
** Globals
*/

// public-facing queue handles
extern pcb_queue_t pcb_freelist;
extern pcb_queue_t ready;
extern pcb_queue_t waiting;
extern pcb_queue_t sleeping;
extern pcb_queue_t zombie;
extern pcb_queue_t sioread;

// pointer to the currently-running process
extern pcb_t *current;

// the process table
extern pcb_t ptable[N_PROCS];

// next available PID
extern uint_t next_pid;

// pointer to the PCB for the 'init' process
extern pcb_t *init_pcb;

// table of state name strings
extern const char state_str[N_STATES][4];

// table of priority name strings
extern const char prio_str[N_PRIOS][5];

// table of queue ordering name strings
extern const char ord_str[N_ORDERINGS][5];

/*
** Prototypes
*/

/**
** Name:    pcb_init
**
** Initialization for the Process module.
*/
void pcb_init(void);

/**
** Name:    pcb_alloc
**
** Allocate a PCB from the list of free PCBs.
**
** @param pcb   Pointer to a pcb_t * where the PCB pointer will be returned.
**
** @return status of the allocation attempt
*/
int pcb_alloc(pcb_t **pcb);

/**
** Name:    pcb_free
**
** Return a PCB to the list of free PCBs.
**
** @param pcb   Pointer to the PCB to be deallocated.
*/
void pcb_free(pcb_t *pcb);

/**
** Name:    pcb_zombify
**
** Turn the indicated process into a Zombie. This function
** does most of the real work for exit() and kill() calls.
** Is also called from the scheduler and dispatcher.
**
** @param pcb   Pointer to the newly-undead PCB
*/
void pcb_zombify(register pcb_t *victim);

/**
** Name:    pcb_cleanup
**
** Reclaim a process' data structures
**
** @param pcb   The PCB to reclaim
*/
void pcb_cleanup(pcb_t *pcb);

/**
** Name:    pcb_find_pid
**
** Locate the PCB for the process with the specified PID
**
** @param pid   The PID to be located
**
** @return Pointer to the PCB, or NULL
*/
pcb_t *pcb_find_pid(uint_t pid);

/**
** Name:    pcb_find_ppid
**
** Locate the PCB for the process with the specified parent
**
** @param pid   The PID to be located
**
** @return Pointer to the PCB, or NULL
*/
pcb_t *pcb_find_ppid(uint_t pid);

/**
** Name:	pcb_queue_reset
**
** Initialize a PCB queue.
**
** @param queue[out]  The queue to be initialized
** @param order[in]   The desired ordering for the queue
**
** @return status of the init request
*/
int pcb_queue_reset(pcb_queue_t queue, enum pcb_queue_order_e style);

/**
** Name:    pcb_queue_empty
**
** Determine whether a queue is empty. Essentially just a wrapper
** for the PCB_QUEUE_EMPTY() macro, for use outside this module.
**
** @param[in] queue  The queue to check
**
** @return true if the queue is empty, else false
*/
bool_t pcb_queue_empty(pcb_queue_t queue);

/**
** Name:    pcb_queue_length
**
** Return the count of elements in the specified queue.
**
** @param[in] queue  The queue to check
**
** @return the count (0 if the queue is empty)
*/
uint_t pcb_queue_length(const pcb_queue_t queue);

/**
** Name:	pcb_queue_insert
**
** Inserts a PCB into the indicated queue.
**
** @param queue[in,out]  The queue to be used
** @param pcb[in]        The PCB to be inserted
**
** @return status of the insertion request
*/
int pcb_queue_insert(pcb_queue_t queue, pcb_t *pcb);

/**
** Name:	pcb_queue_peek
**
** Return the first PCB from the indicated queue, but don't
** remove it from the queue
**
** @param queue[in]  The queue to be used
**
** @return the PCB pointer, or NULL if the queue is empty
*/
pcb_t *pcb_queue_peek(const pcb_queue_t queue);

/**
** Name:	pcb_queue_remove
**
** Remove the first PCB from the indicated queue.
**
** @param queue[in,out]  The queue to be used
** @param pcb[out]       Pointer to where the PCB pointer will be saved
**
** @return status of the removal request
*/
int pcb_queue_remove(pcb_queue_t queue, pcb_t **pcb);

/**
** Name:	pcb_queue_remove_this
**
** Remove the specified PCB from the indicated queue.
**
** @param queue[in,out]  The queue to be used
** @param pcb[in]        Pointer to the PCB to be removed
**
** @return status of the removal request
*/
int pcb_queue_remove_this(pcb_queue_t queue, pcb_t *pcb);

/*
** Scheduler routines
*/

/**
** schedule(pcb)
**
** Schedule the supplied process
**
** @param pcb   Pointer to the PCB of the process to be scheduled
*/
void schedule(pcb_t *pcb);

/**
** dispatch()
**
** Select the next process to receive the CPU
*/
void dispatch(void);

/*
** Debugging/tracing routines
*/

/**
** Name:	ctx_dump
**
** Dumps the contents of this process context to the console
**
** @param msg[in]   An optional message to print before the dump
** @param c[in]     The context to dump out
*/
void ctx_dump(const char *msg, register context_t *c);

/**
** Name:	ctx_dump_all
**
** dump the process context for all active processes
**
** @param msg[in]  Optional message to print
*/
void ctx_dump_all(const char *msg);

/**
** Name:	pcb_dump
**
** Dumps the contents of this PCB to the console
**
** @param msg[in]  An optional message to print before the dump
** @param p[in]    The PCB to dump
** @param all[in]  Dump all the contents?
*/
void pcb_dump(const char *msg, register pcb_t *p, bool_t all);

/**
** Name:	pcb_queue_dump
**
** Dump the contents of the specified queue to the console
**
** @param msg[in]       An optional message to print before the dump
** @param queue[in]     The queue to dump
** @param contents[in]  Also dump (some) contents?
*/
void pcb_queue_dump(const char *msg, pcb_queue_t queue, bool_t contents);

/**
** Name:	ptable_dump
**
** dump the contents of the "active processes" table
**
** @param msg[in]  Optional message to print
** @param all[in]  Dump all or only part of the relevant data
*/
void ptable_dump(const char *msg, bool_t all);

/**
** Name:	ptable_dump_counts
**
** Prints basic information about the process table (number of
** entries, number with each process state, etc.).
*/
void ptable_dump_counts(void);

#endif /* !ASM_SRC */

#endif
