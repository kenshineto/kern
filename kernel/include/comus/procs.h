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
#include <elf.h>

#define PCB_REG(pcb, x) ((pcb)->regs->x)
#define PCB_RET(pcb) ((pcb)->regs->rax)
#define PCB_ARG1(pcb) PCB_REG((pcb), rdi)
#define PCB_ARG2(pcb) PCB_REG((pcb), rsi)
#define PCB_ARG3(pcb) PCB_REG((pcb), rdx)
#define PCB_ARG4(pcb) PCB_REG((pcb), rcx)

/// process id
typedef unsigned short pid_t;

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

/// process control block
struct pcb {
	// context
	struct cpu_regs *regs;
	mem_ctx_t memctx;

	// metadata
	pid_t pid;
	struct pcb *parent;
	enum proc_state state;
	size_t priority;
	size_t ticks;

	// elf metadata
	Elf64_Ehdr elf_header;
	Elf64_Phdr elf_segments[N_ELF_SEGMENTS];
	Elf64_Half n_elf_segments;

	// queue linkage
	struct pcb *next; // next PDB in queue

	// process state information
	uint64_t wakeup;
	uint8_t exit_status;
};

/// ordering of pcb queues
enum pcb_queue_order {
	O_PCB_FIFO,
	O_PCB_PRIO,
	O_PCB_PID,
	O_PCB_WAKEUP,
	// sentinel
	N_PCB_ORDERINGS,
};

/// pcb queue structure
typedef struct pcb_queue_s *pcb_queue_t;

/// public facing pcb queues
extern pcb_queue_t pcb_freelist;
extern pcb_queue_t ready;
extern pcb_queue_t waiting;
extern pcb_queue_t sleeping;
extern pcb_queue_t zombie;

/// pointer to the currently-running process
extern struct pcb *current_pcb;
/// pointer to the pcb for the 'init' process
extern struct pcb *init_pcb;
/// the process table
extern struct pcb ptable[N_PROCS];

/// next avaliable pid
extern pid_t next_pid;

/**
 * Initialization for the process module
 */
void pcb_init(void);

/**
 * allocate a PCB from the free list
 *
 * @returns 0 on success or non zero error code
 */
int pcb_alloc(struct pcb **pcb);

/**
 * free an allocted PCB back to the free list
 *
 * @param pcb - pointer to the PCB to be deallocated
 */
void pcb_free(struct pcb *pcb);

/**
 * turn the indicated process into a zombie
 *
 * @param pcb - pointer to the newly-undead PCB
 */
void pcb_zombify(struct pcb *victim);

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
int pcb_queue_pop(pcb_queue_t queue, struct pcb **pcb);

/**
 * Remove the specified PCB from the indicated queue.
 *
 * @param queue[in,out]  The queue to be used
 * @param pcb[in]        Pointer to the PCB to be removed
 * @return status of the removal request
 */
int pcb_queue_remove(pcb_queue_t queue, struct pcb *pcb);

/**
 * Schedule the supplied process
 *
 * @param pcb   Pointer to the PCB of the process to be scheduled
 */
void schedule(struct pcb *pcb);

/**
 * Select the next process to receive the CPU
 */
__attribute__((noreturn))
void dispatch(void);

#endif /* procs.h */
