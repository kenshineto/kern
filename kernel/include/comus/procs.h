/**
 * @file procs.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 * @author CSCI-452 class of 20245
 *
 */

#ifndef _PROCS_H
#define _PROCS_H

#include <comus/cpu.h>
#include <comus/limits.h>
#include <comus/memory.h>
#include <lib.h>

/// process states
enum proc_state {
	// pre-viable
	PROC_STATE_UNUSED = 0,
	PROC_STATE_NEW,
	// runnable
	PROC_STATE_READY,
	PROC_STATE_RUNNING,
	// runnable, but waiting for some event
	PROC_STATE_SLEEPING,
	PROC_STATE_BLOCKED,
	PROC_STATE_WAITING,
	// no longer runnalbe
	PROC_STATE_KILLED,
	PROC_STATE_ZOMBIE,
	// sentinel
	N_PROC_STATES,
};

/// process priority
enum proc_priority {
	PROC_PRIO_HIGH,
	PROC_PRIO_STD,
	PROC_PRIO_LOW,
	PROC_PRIO_DEFERRED,
	// sentinel
	N_PROC_PRIOS,
};

/// process quantum length
/// values are number of clock ticks
enum proc_quantum {
	PROC_QUANTUM_SHORT = 1,
	PROC_QUANTUM_STANDARD = 3,
	PROC_QUANTUM_LONG = 5,
};

/// program section information
struct proc_section {
	uint64_t length;
	uint64_t addr;
};

#define SECT_L1 0
#define SECT_L2 1
#define SECT_L3 2
#define SECT_STACK 3
#define N_SECTS 4
#define N_LOADABLE 3

/// pid type
typedef unsigned short pid_t;

/// process control block
struct pcb {
	// process context
	mem_ctx_t memctx;
	struct cpu_regs *regs;

	// vm information
	struct proc_section sects[N_SECTS];

	// queue linkage
	struct pcb *next; // next PDB in queue

	// process state information
	struct pcb *parent; // pointer to PCB of our parent process
	uint64_t wakeup; // wakeup time, for sleeping processes
	uint8_t exit_status; // termination status, for parent's use

	// process metadata
	pid_t pid; // pid of this process
	enum proc_state state; // process' current state
	enum proc_priority priority; // process priority level
	size_t ticks; // remaining ticks in this time slice
};

#define PCB_REG(pcb, x) ((pcb)->regs->x)
#define PCB_RET(pcb) ((pcb)->regs->rax)
#define PCB_ARG(pcb, n) (((uint64_t *)(((pcb)->regs) + 1))[(n)])

/// pcb queue structure
typedef struct pcb_queue_s *pcb_queue_t;

/// ordering of pcb queues
enum pcb_queue_order {
	O_PCB_FIFO,
	O_PCB_PRIO,
	O_PCB_PID,
	O_PCB_WAKEUP,
	// sentinel
	N_PCB_ORDERINGS,
};

/// public facing pcb queues
extern pcb_queue_t pcb_freelist;
extern pcb_queue_t ready;
extern pcb_queue_t waiting;
extern pcb_queue_t sleeping;
extern pcb_queue_t zombie;

/// pointer to the currently-running process
extern struct pcb *current;

/// the process table
extern struct pcb ptable[N_PROCS];

/// next avaliable pid
extern pid_t next_pid;

/// pointer to the pcb for the 'init' process
extern struct pcb *init_pcb;

/// table of state name strings
extern const char *proc_state_str[N_PROC_STATES];

/// table of priority name strings
extern const char *proc_prio_str[N_PROC_PRIOS];

/// table of queue ordering name strings
extern const char *pcb_ord_str[N_PCB_ORDERINGS];

/**
 * Initialization for the process module
 */
void pcb_init(void);

/**
 * Allocate a PCB from the list of free PCBs
 */
int pcb_alloc(struct pcb **pcb);

/**
 * Return a PCB to the list of free PCBs.
 *
 * @param pcb   Pointer to the PCB to be deallocated.
 */
void pcb_free(struct pcb *pcb);

/**
 * Turn the indicated process into a Zombie. This function
 * does most of the real work for exit() and kill() calls.
 * Is also called from the scheduler and dispatcher.
 *
 * @param pcb - pointer to the newly-undead PCB
 */
void pcb_zombify(register struct pcb *victim);

/**
 * Reclaim a process' data structures
 *
 * @param pcb - the PCB to reclaim
 */
void pcb_cleanup(struct pcb *pcb);

/**
 * Locate the PCB for the process with the specified PID
 *
 * @param pid - the PID to be located
 * @return Pointer to the PCB, or NULL
 */
struct pcb *pcb_find_pid(pid_t pid);

/**
 * Locate the PCB for the process with the specified parent
 *
 * @param pid   The PID to be located
 * @return Pointer to the PCB, or NULL
 */
struct pcb *pcb_find_ppid(pid_t pid);

/**
 * Initialize a PCB queue.
 *
 * @param queue[out]  The queue to be initialized
 * @param order[in]   The desired ordering for the queue
 *
 * @return status of the init request
 */
int pcb_queue_reset(pcb_queue_t queue, enum pcb_queue_order style);

/**
 * Determine whether a queue is empty. Essentially just a wrapper
 * for the PCB_QUEUE_EMPTY() macro, for use outside this module.
 *
 * @param[in] queue  The queue to check
 * @return true if the queue is empty, else false
 */
bool pcb_queuempty(pcb_queue_t queue);

/**
 * Return the count of elements in the specified queue.
 *
 * @param[in] queue  The queue to check
 * @return the count (0 if the queue is empty)
 */
size_t pcb_queue_length(const pcb_queue_t queue);

/**
 * Inserts a PCB into the indicated queue.
 *
 * @param queue[in,out]  The queue to be used
 * @param pcb[in]        The PCB to be inserted
 * @return status of the insertion request
 */
int pcb_queue_insert(pcb_queue_t queue, struct pcb *pcb);

/**
 * Return the first PCB from the indicated queue, but don't
 * remove it from the queue
 *
 * @param queue[in]  The queue to be used
 * @return the PCB pointer, or NULL if the queue is empty
 */
struct pcb *pcb_queue_peek(const pcb_queue_t queue);

/**
 * Remove the first PCB from the indicated queue.
 *
 * @param queue[in,out]  The queue to be used
 * @param pcb[out]       Pointer to where the PCB pointer will be saved
 * @return status of the removal request
 */
int pcb_queue_remove(pcb_queue_t queue, struct pcb **pcb);

/**
 * Remove the specified PCB from the indicated queue.
 *
 * @param queue[in,out]  The queue to be used
 * @param pcb[in]        Pointer to the PCB to be removed
 * @return status of the removal request
 */
int pcb_queue_remove_this(pcb_queue_t queue, struct pcb *pcb);

/**
 * Schedule the supplied process
 *
 * @param pcb   Pointer to the PCB of the process to be scheduled
 */
void schedule(struct pcb *pcb);

/**
 * Select the next process to receive the CPU
 */
void dispatch(void);

/**
 * Dumps the contents of this process context to the console
 *
 * @param msg[in]   An optional message to print before the dump
 * @param c[in]     The context to dump out
 */
void ctx_dump(const char *msg, register struct cpu_regs *c);

/**
 * dump the process context for all active processes
 *
 * @param msg[in]  Optional message to print
 */
void ctx_dump_all(const char *msg);

/**
 * Dumps the contents of this PCB to the console
 *
 * @param msg[in]  An optional message to print before the dump
 * @param p[in]    The PCB to dump
 * @param all[in]  Dump all the contents?
 */
void pcb_dump(const char *msg, register struct pcb *p, bool all);

/**
 * Dump the contents of the specified queue to the console
 *
 * @param msg[in]       An optional message to print before the dump
 * @param queue[in]     The queue to dump
 * @param contents[in]  Also dump (some) contents?
 */
void pcb_queue_dump(const char *msg, pcb_queue_t queue, bool contents);

/**
 * dump the contents of the "active processes" table
 *
 * @param msg[in]  Optional message to print
 * @param all[in]  Dump all or only part of the relevant data
 */
void ptable_dump(const char *msg, bool all);

/**
 * Prints basic information about the process table (number of
 * entries, number with each process state, etc.).
 */
void ptable_dump_counts(void);

#endif /* procs.h */
