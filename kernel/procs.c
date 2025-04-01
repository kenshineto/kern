/*
** @file	procs.c
**
** @author	CSCI-452 class of 20245
**
** @brief	Process-related implementations
*/

#define KERNEL_SRC

#include <common.h>

#include <procs.h>
#include <user.h>

/*
** PRIVATE DEFINITIONS
*/

// determine if a queue is empty; assumes 'q' is a valid pointer
#define PCB_QUEUE_EMPTY(q) ((q)->head == NULL)

/*
** PRIVATE DATA TYPES
*/

/*
** PCB Queue structure
**
** Opaque to the rest of the kernel
**
** Typedef'd in the header: typedef struct pcb_queue_s *pcb_queue_t;
*/
struct pcb_queue_s {
	pcb_t *head;
	pcb_t *tail;
	enum pcb_queue_order_e order;
};

/*
** PRIVATE GLOBAL VARIABLES
*/

// collection of queues
static struct pcb_queue_s pcb_freelist_queue;
static struct pcb_queue_s ready_queue;
static struct pcb_queue_s waiting_queue;
static struct pcb_queue_s sleeping_queue;
static struct pcb_queue_s zombie_queue;
static struct pcb_queue_s sioread_queue;

/*
** PUBLIC GLOBAL VARIABLES
*/

// public-facing queue handles
pcb_queue_t pcb_freelist;
pcb_queue_t ready;
pcb_queue_t waiting;
pcb_queue_t sleeping;
pcb_queue_t zombie;
pcb_queue_t sioread;

// pointer to the currently-running process
pcb_t *current;

// the process table
pcb_t ptable[N_PROCS];

// next available PID
uint_t next_pid;

// pointer to the PCB for the 'init' process
pcb_t *init_pcb;

// table of state name strings
const char state_str[N_STATES][4] = {
	[STATE_UNUSED] = "Unu", // "Unused"
	[STATE_NEW] = "New",
	[STATE_READY] = "Rdy", // "Ready"
	[STATE_RUNNING] = "Run", // "Running"
	[STATE_SLEEPING] = "Slp", // "Sleeping"
	[STATE_BLOCKED] = "Blk", // "Blocked"
	[STATE_WAITING] = "Wat", // "Waiting"
	[STATE_KILLED] = "Kil", // "Killed"
	[STATE_ZOMBIE] = "Zom" // "Zombie"
};

// table of priority name strings
const char prio_str[N_PRIOS][5] = { [PRIO_HIGH] = "High",
									[PRIO_STD] = "User",
									[PRIO_LOW] = "Low ",
									[PRIO_DEFERRED] = "Def " };

// table of queue ordering name strings
const char ord_str[N_PRIOS][5] = { [O_FIFO] = "FIFO",
								   [O_PRIO] = "PRIO",
								   [O_PID] = "PID ",
								   [O_WAKEUP] = "WAKE" };

/*
** PRIVATE FUNCTIONS
*/

/**
** Priority search functions. These are used to traverse a supplied
** queue looking for the queue entry that would precede the supplied
** PCB when that PCB is inserted into the queue.
**
** Variations:
**     find_prev_wakeup()     compares wakeup times
**     find_prev_priority()   compares process priorities
**     find_prev_pid()        compares PIDs
**
** Each assumes the queue should be in ascending order by the specified
** comparison value.
**
** @param[in] queue  The queue to search
** @param[in] pcb    The PCB to look for
**
** @return a pointer to the predecessor in the queue, or NULL if
** this PCB would be at the beginning of the queue.
*/
static pcb_t *find_prev_wakeup(pcb_queue_t queue, pcb_t *pcb)
{
	// sanity checks!
	assert1(queue != NULL);
	assert1(pcb != NULL);

	pcb_t *prev = NULL;
	pcb_t *curr = queue->head;

	while (curr != NULL && curr->wakeup <= pcb->wakeup) {
		prev = curr;
		curr = curr->next;
	}

	return prev;
}

static pcb_t *find_prev_priority(pcb_queue_t queue, pcb_t *pcb)
{
	// sanity checks!
	assert1(queue != NULL);
	assert1(pcb != NULL);

	pcb_t *prev = NULL;
	pcb_t *curr = queue->head;

	while (curr != NULL && curr->priority <= pcb->priority) {
		prev = curr;
		curr = curr->next;
	}

	return prev;
}

static pcb_t *find_prev_pid(pcb_queue_t queue, pcb_t *pcb)
{
	// sanity checks!
	assert1(queue != NULL);
	assert1(pcb != NULL);

	pcb_t *prev = NULL;
	pcb_t *curr = queue->head;

	while (curr != NULL && curr->pid <= pcb->pid) {
		prev = curr;
		curr = curr->next;
	}

	return prev;
}

/*
** PUBLIC FUNCTIONS
*/

// a macro to simplify queue setup
#define QINIT(q, s)                           \
	q = &q##_queue;                           \
	if (pcb_queue_reset(q, s) != SUCCESS) {   \
		PANIC(0, "pcb_init can't reset " #q); \
	}

/**
** Name:	pcb_init
**
** Initialization for the Process module.
*/
void pcb_init(void)
{
#if TRACING_INIT
	cio_puts(" Procs");
#endif

	// there is no current process
	current = NULL;

	// set up the external links to the queues
	QINIT(pcb_freelist, O_FIFO);
	QINIT(ready, O_PRIO);
	QINIT(waiting, O_PID);
	QINIT(sleeping, O_WAKEUP);
	QINIT(zombie, O_PID);
	QINIT(sioread, O_FIFO);

	/*
	** We statically allocate our PCBs, so we need to add them
	** to the freelist before we can use them. If this changes
	** so that we dynamicallyl allocate PCBs, this step either
	** won't be required, or could be used to pre-allocate some
	** number of PCB structures for future use.
	*/

	pcb_t *ptr = ptable;
	for (int i = 0; i < N_PROCS; ++i) {
		pcb_free(ptr);
		++ptr;
	}
}

/**
** Name:	pcb_alloc
**
** Allocate a PCB from the list of free PCBs.
**
** @param pcb   Pointer to a pcb_t * where the PCB pointer will be returned.
**
** @return status of the allocation attempt
*/
int pcb_alloc(pcb_t **pcb)
{
	// sanity check!
	assert1(pcb != NULL);

	// remove the first PCB from the free list
	pcb_t *tmp;
	if (pcb_queue_remove(pcb_freelist, &tmp) != SUCCESS) {
		return E_NO_PCBS;
	}

	*pcb = tmp;
	return SUCCESS;
}

/**
** Name:	pcb_free
**
** Return a PCB to the list of free PCBs.
**
** @param pcb   Pointer to the PCB to be deallocated.
*/
void pcb_free(pcb_t *pcb)
{
	if (pcb != NULL) {
		// mark the PCB as available
		pcb->state = STATE_UNUSED;

		// add it to the free list
		int status = pcb_queue_insert(pcb_freelist, pcb);

		// if that failed, we're in trouble
		if (status != SUCCESS) {
			sprint(b256, "pcb_free(0x%08x) status %d", (uint32_t)pcb, status);
			PANIC(0, b256);
		}
	}
}

/**
** Name:	pcb_zombify
**
** Turn the indicated process into a Zombie. This function
** does most of the real work for exit() and kill() calls.
** Is also called from the scheduler and dispatcher.
**
** @param pcb   Pointer to the newly-undead PCB
*/
void pcb_zombify(register pcb_t *victim)
{
	// should this be an error?
	if (victim == NULL) {
		return;
	}

	// every process must have a parent, even if it's 'init'
	assert(victim->parent != NULL);

	/*
	** We need to locate the parent of this process.  We also need
	** to reparent any children of this process.  We do these in
	** a single loop.
	*/
	pcb_t *parent = victim->parent;
	pcb_t *zchild = NULL;

	// two PIDs we will look for
	uint_t vicpid = victim->pid;

	// speed up access to the process table entries
	register pcb_t *curr = ptable;

	for (int i = 0; i < N_PROCS; ++i, ++curr) {
		// make sure this is a valid entry
		if (curr->state == STATE_UNUSED) {
			continue;
		}

		// if this is our parent, just keep going - we continue
		// iterating to find all the children of this process.
		if (curr == parent) {
			continue;
		}

		if (curr->parent == victim) {
			// found a child - reparent it
			curr->parent = init_pcb;

			// see if this child is already undead
			if (curr->state == STATE_ZOMBIE) {
				// if it's already a zombie, remember it, so we
				// can pass it on to 'init'; also, if there are
				// two or more zombie children, it doesn't matter
				// which one we pick here, as the others will be
				// collected when 'init' loops
				zchild = curr;
			}
		}
	}

	/*
	** If we found a child that was already terminated, we need to
	** wake up the init process if it's already waiting.
	**
	** Note: we only need to do this for one Zombie child process -
	** init will loop and collect the others after it finishes with
	** this one.
	**
	** Also note: it's possible that the exiting process' parent is
	** also init, which means we're letting one of zombie children
	** of the exiting process be cleaned up by init before the
	** existing process itself is cleaned up by init. This will work,
	** because after init cleans up the zombie, it will loop and
	** call waitpid() again, by which time this exiting process will
	** be marked as a zombie.
	*/
	if (zchild != NULL && init_pcb->state == STATE_WAITING) {
		// dequeue the zombie
		assert(pcb_queue_remove_this(zombie, zchild) == SUCCESS);

		assert(pcb_queue_remove_this(waiting, init_pcb) == SUCCESS);

		// intrinsic return value is the PID
		RET(init_pcb) = zchild->pid;

		// may also want to return the exit status
		int32_t *ptr = (int32_t *)ARG(init_pcb, 2);

		if (ptr != NULL) {
			// ********************************************************
			// ** Potential VM issue here!  This code assigns the exit
			// ** status into a variable in the parent's address space.
			// ** This works in the baseline because we aren't using
			// ** any type of memory protection.  If address space
			// ** separation is implemented, this code will very likely
			// ** STOP WORKING, and will need to be fixed.
			// ********************************************************
			*ptr = zchild->exit_status;
		}

		// all done - schedule 'init', and clean up the zombie
		schedule(init_pcb);
		pcb_cleanup(zchild);
	}

	/*
	** Now, deal with the parent of this process. If the parent is
	** already waiting, just wake it up and clean up this process.
	** Otherwise, this process becomes a zombie.
	**
	** Note: if the exiting process' parent is init and we just woke
	** init up to deal with a zombie child of the exiting process,
	** init's status won't be Waiting any more, so we don't have to
	** worry about it being scheduled twice.
	*/

	if (parent->state == STATE_WAITING) {
		// verify that the parent is either waiting for this process
		// or is waiting for any of its children
		uint32_t target = ARG(parent, 1);

		if (target == 0 || target == vicpid) {
			// the parent is waiting for this child or is waiting
			// for any of its children, so we can wake it up.

			// intrinsic return value is the PID
			RET(parent) = vicpid;

			// may also want to return the exit status
			int32_t *ptr = (int32_t *)ARG(parent, 2);

			if (ptr != NULL) {
				// ********************************************************
				// ** Potential VM issue here!  This code assigns the exit
				// ** status into a variable in the parent's address space.
				// ** This works in the baseline because we aren't using
				// ** any type of memory protection.  If address space
				// ** separation is implemented, this code will very likely
				// ** STOP WORKING, and will need to be fixed.
				// ********************************************************
				*ptr = victim->exit_status;
			}

			// all done - schedule the parent, and clean up the zombie
			schedule(parent);
			pcb_cleanup(victim);

			return;
		}
	}

	/*
	** The parent isn't waiting OR is waiting for a specific child
	** that isn't this exiting process, so we become a Zombie.
	**
	** This code assumes that Zombie processes are *not* in
	** a queue, but instead are just in the process table with
	** a state of 'Zombie'.  This simplifies life immensely,
	** because we won't need to dequeue it when it is collected
	** by its parent.
	*/

	victim->state = STATE_ZOMBIE;
	assert(pcb_queue_insert(zombie, victim) == SUCCESS);

	/*
	** Note: we don't call _dispatch() here - we leave that for
	** the calling routine, as it's possible we don't need to
	** choose a new current process.
	*/
}

/**
** Name:	pcb_cleanup
**
** Reclaim a process' data structures
**
** @param pcb   The PCB to reclaim
*/
void pcb_cleanup(pcb_t *pcb)
{
#if TRACING_PCB
	cio_printf("** pcb_cleanup(0x%08x)\n", (uint32_t)pcb);
#endif

	// avoid deallocating a NULL pointer
	if (pcb == NULL) {
		// should this be an error?
		return;
	}

	// we need to release all the VM data structures and frames
	user_cleanup(pcb);

	// release the PCB itself
	pcb_free(pcb);
}

/**
** Name:	pcb_find_pid
**
** Locate the PCB for the process with the specified PID
**
** @param pid   The PID to be located
**
** @return Pointer to the PCB, or NULL
*/
pcb_t *pcb_find_pid(uint_t pid)
{
	// must be a valid PID
	if (pid < 1) {
		return NULL;
	}

	// scan the process table
	pcb_t *p = ptable;

	for (int i = 0; i < N_PROCS; ++i, ++p) {
		if (p->pid == pid && p->state != STATE_UNUSED) {
			return p;
		}
	}

	// didn't find it!
	return NULL;
}

/**
** Name:	pcb_find_ppid
**
** Locate the PCB for the process with the specified parent
**
** @param pid   The PID to be located
**
** @return Pointer to the PCB, or NULL
*/
pcb_t *pcb_find_ppid(uint_t pid)
{
	// must be a valid PID
	if (pid < 1) {
		return NULL;
	}

	// scan the process table
	pcb_t *p = ptable;

	for (int i = 0; i < N_PROCS; ++i, ++p) {
		assert1(p->parent != NULL);
		if (p->parent->pid == pid && p->parent->state != STATE_UNUSED) {
			return p;
		}
	}

	// didn't find it!
	return NULL;
}

/**
** Name:    pcb_queue_reset
**
** Initialize a PCB queue. We assume that whatever data may be
** in the queue structure can be overwritten.
**
** @param queue[out]  The queue to be initialized
** @param order[in]   The desired ordering for the queue
**
** @return status of the init request
*/
int pcb_queue_reset(pcb_queue_t queue, enum pcb_queue_order_e style)
{
	// sanity check
	assert1(queue != NULL);

	// make sure the style is valid
	if (style < O_FIRST_STYLE || style > O_LAST_STYLE) {
		return E_BAD_PARAM;
	}

	// reset the queue
	queue->head = queue->tail = NULL;
	queue->order = style;

	return SUCCESS;
}

/**
** Name:	pcb_queue_empty
**
** Determine whether a queue is empty. Essentially just a wrapper
** for the PCB_QUEUE_EMPTY() macro, for use outside this module.
**
** @param[in] queue  The queue to check
**
** @return true if the queue is empty, else false
*/
bool_t pcb_queue_empty(pcb_queue_t queue)
{
	// if there is no queue, blow up
	assert1(queue != NULL);

	return PCB_QUEUE_EMPTY(queue);
}

/**
** Name:    pcb_queue_length
**
** Return the count of elements in the specified queue.
**
** @param[in] queue  The queue to check
**
** @return the count (0 if the queue is empty)
*/
uint_t pcb_queue_length(const pcb_queue_t queue)
{
	// sanity check
	assert1(queue != NULL);

	// this is pretty simple
	register pcb_t *tmp = queue->head;
	register int num = 0;

	while (tmp != NULL) {
		++num;
		tmp = tmp->next;
	}

	return num;
}

/**
** Name:    pcb_queue_insert
**
** Inserts a PCB into the indicated queue.
**
** @param queue[in,out]  The queue to be used
** @param pcb[in]        The PCB to be inserted
**
** @return status of the insertion request
*/
int pcb_queue_insert(pcb_queue_t queue, pcb_t *pcb)
{
	// sanity checks
	assert1(queue != NULL);
	assert1(pcb != NULL);

	// if this PCB is already in a queue, we won't touch it
	if (pcb->next != NULL) {
		// what to do? we let the caller decide
		return E_BAD_PARAM;
	}

	// is the queue empty?
	if (queue->head == NULL) {
		queue->head = queue->tail = pcb;
		return SUCCESS;
	}
	assert1(queue->tail != NULL);

	// no, so we need to search it
	pcb_t *prev = NULL;

	// find the predecessor node
	switch (queue->order) {
	case O_FIFO:
		prev = queue->tail;
		break;
	case O_PRIO:
		prev = find_prev_priority(queue, pcb);
		break;
	case O_PID:
		prev = find_prev_pid(queue, pcb);
		break;
	case O_WAKEUP:
		prev = find_prev_wakeup(queue, pcb);
		break;
	default:
		// do we need something more specific here?
		return E_BAD_PARAM;
	}

	// OK, we found the predecessor node; time to do the insertion

	if (prev == NULL) {
		// there is no predecessor, so we're
		// inserting at the front of the queue
		pcb->next = queue->head;
		if (queue->head == NULL) {
			// empty queue!?! - should we panic?
			queue->tail = pcb;
		}
		queue->head = pcb;

	} else if (prev->next == NULL) {
		// append at end
		prev->next = pcb;
		queue->tail = pcb;

	} else {
		// insert between prev & prev->next
		pcb->next = prev->next;
		prev->next = pcb;
	}

	return SUCCESS;
}

/**
** Name:    pcb_queue_remove
**
** Remove the first PCB from the indicated queue.
**
** @param queue[in,out]  The queue to be used
** @param pcb[out]       Pointer to where the PCB pointer will be saved
**
** @return status of the removal request
*/
int pcb_queue_remove(pcb_queue_t queue, pcb_t **pcb)
{
	//sanity checks
	assert1(queue != NULL);
	assert1(pcb != NULL);

	// can't get anything if there's nothing to get!
	if (PCB_QUEUE_EMPTY(queue)) {
		return E_EMPTY_QUEUE;
	}

	// take the first entry from the queue
	pcb_t *tmp = queue->head;
	queue->head = tmp->next;

	// disconnect it completely
	tmp->next = NULL;

	// was this the last thing in the queue?
	if (queue->head == NULL) {
		// yes, so clear the tail pointer for consistency
		queue->tail = NULL;
	}

	// save the pointer
	*pcb = tmp;

	return SUCCESS;
}

/**
** Name:    pcb_queue_remove_this
**
** Remove the specified PCB from the indicated queue.
**
** We don't return the removed pointer, because the calling
** routine must already have it (because it was supplied
** to us in the call).
**
** @param queue[in,out]  The queue to be used
** @param pcb[in]        Pointer to the PCB to be removed
**
** @return status of the removal request
*/
int pcb_queue_remove_this(pcb_queue_t queue, pcb_t *pcb)
{
	//sanity checks
	assert1(queue != NULL);
	assert1(pcb != NULL);

	// can't get anything if there's nothing to get!
	if (PCB_QUEUE_EMPTY(queue)) {
		return E_EMPTY_QUEUE;
	}

	// iterate through the queue until we find the desired PCB
	pcb_t *prev = NULL;
	pcb_t *curr = queue->head;

	while (curr != NULL && curr != pcb) {
		prev = curr;
		curr = curr->next;
	}

	// case  prev  curr  next   interpretation
	// ====  ====  ====  ====   ============================
	//   1.    0     0    --    *** CANNOT HAPPEN ***
	//   2.    0    !0     0    removing only element
	//   3.    0    !0    !0    removing first element
	//   4.   !0     0    --    *** NOT FOUND ***
	//   5.   !0    !0     0    removing from end
	//   6.   !0    !0    !0    removing from middle

	if (curr == NULL) {
		// case 1
		assert(prev != NULL);
		// case 4
		return E_NOT_FOUND;
	}

	// connect predecessor to successor
	if (prev != NULL) {
		// not the first element
		// cases 5 and 6
		prev->next = curr->next;
	} else {
		// removing first element
		// cases 2 and 3
		queue->head = curr->next;
	}

	// if this was the last node (cases 2 and 5),
	// also need to reset the tail pointer
	if (curr->next == NULL) {
		// if this was the only entry (2), prev is NULL,
		// so this works for that case, too
		queue->tail = prev;
	}

	// unlink current from queue
	curr->next = NULL;

	// there's a possible consistancy problem here if somehow
	// one of the queue pointers is NULL and the other one
	// is not NULL

	assert1((queue->head == NULL && queue->tail == NULL) ||
			(queue->head != NULL && queue->tail != NULL));

	return SUCCESS;
}

/**
** Name:    pcb_queue_peek
**
** Return the first PCB from the indicated queue, but don't
** remove it from the queue.
**
** @param queue[in]  The queue to be used
**
** @return the PCB poiner, or NULL if the queue is empty
*/
pcb_t *pcb_queue_peek(const pcb_queue_t queue)
{
	//sanity check
	assert1(queue != NULL);

	// can't get anything if there's nothing to get!
	if (PCB_QUEUE_EMPTY(queue)) {
		return NULL;
	}

	// just return the first entry from the queue
	return queue->head;
}

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
void schedule(pcb_t *pcb)
{
	// sanity check
	assert1(pcb != NULL);

	// check for a killed process
	if (pcb->state == STATE_KILLED) {
		// TODO figure out what to do now
		return;
	}

	// mark it as ready
	pcb->state = STATE_READY;

	// add it to the ready queue
	if (pcb_queue_insert(ready, pcb) != SUCCESS) {
		PANIC(0, "schedule insert fail");
	}
}

/**
** dispatch()
**
** Select the next process to receive the CPU
*/
void dispatch(void)
{
	// verify that there is no current process
	assert(current == NULL);

	// grab whoever is at the head of the queue
	int status = pcb_queue_remove(ready, &current);
	if (status != SUCCESS) {
		sprint(b256, "dispatch queue remove failed, code %d", status);
		PANIC(0, b256);
	}

	// set the process up for success
	current->state = STATE_RUNNING;
	current->ticks = QUANTUM_STANDARD;
}

/*
** Debugging/tracing routines
*/

/**
** ctx_dump(msg,context)
**
** Dumps the contents of this process context to the console
**
** @param msg[in]   An optional message to print before the dump
** @param c[in]     The context to dump out
*/
void ctx_dump(const char *msg, register context_t *c)
{
	// first, the message (if there is one)
	if (msg) {
		cio_puts(msg);
	}

	// the pointer
	cio_printf(" @ %08x: ", (uint32_t)c);

	// if it's NULL, why did you bother calling me?
	if (c == NULL) {
		cio_puts(" NULL???\n");
		return;
	}

	// now, the contents
	cio_printf("ss %04x gs %04x fs %04x es %04x ds %04x cs %04x\n",
			   c->ss & 0xff, c->gs & 0xff, c->fs & 0xff, c->es & 0xff,
			   c->ds & 0xff, c->cs & 0xff);
	cio_printf("  edi %08x esi %08x ebp %08x esp %08x\n", c->edi, c->esi,
			   c->ebp, c->esp);
	cio_printf("  ebx %08x edx %08x ecx %08x eax %08x\n", c->ebx, c->edx,
			   c->ecx, c->eax);
	cio_printf("  vec %08x cod %08x eip %08x eflags %08x\n", c->vector, c->code,
			   c->eip, c->eflags);
}

/**
** ctx_dump_all(msg)
**
** dump the process context for all active processes
**
** @param msg[in]  Optional message to print
*/
void ctx_dump_all(const char *msg)
{
	if (msg != NULL) {
		cio_puts(msg);
	}

	int n = 0;
	register pcb_t *pcb = ptable;
	for (int i = 0; i < N_PROCS; ++i, ++pcb) {
		if (pcb->state != STATE_UNUSED) {
			++n;
			cio_printf("%2d(%d): ", n, pcb->pid);
			ctx_dump(NULL, pcb->context);
		}
	}
}

/**
** pcb_dump(msg,pcb,all)
**
** Dumps the contents of this PCB to the console
**
** @param msg[in]  An optional message to print before the dump
** @param pcb[in]  The PCB to dump
** @param all[in]  Dump all the contents?
*/
void pcb_dump(const char *msg, register pcb_t *pcb, bool_t all)
{
	// first, the message (if there is one)
	if (msg) {
		cio_puts(msg);
	}

	// the pointer
	cio_printf(" @ %08x:", (uint32_t)pcb);

	// if it's NULL, why did you bother calling me?
	if (pcb == NULL) {
		cio_puts(" NULL???\n");
		return;
	}

	cio_printf(" %d %s", pcb->pid,
			   pcb->state >= N_STATES ? "???" : state_str[pcb->state]);

	if (!all) {
		// just printing IDs and states on one line
		return;
	}

	// now, the rest of the contents
	cio_printf(" %s",
			   pcb->priority >= N_PRIOS ? "???" : prio_str[pcb->priority]);

	cio_printf(" ticks %u xit %d wake %08x\n", pcb->ticks, pcb->exit_status,
			   pcb->wakeup);

	cio_printf(" parent %08x", (uint32_t)pcb->parent);
	if (pcb->parent != NULL) {
		cio_printf(" (%u)", pcb->parent->pid);
	}

	cio_printf(" next %08x context %08x pde %08x", (uint32_t)pcb->next,
			   (uint32_t)pcb->context, (uint32_t)pcb->pdir);

	cio_putchar('\n');
}

/**
** pcb_queue_dump(msg,queue,contents)
**
** @param msg[in]       Optional message to print
** @param queue[in]     The queue to dump
** @param contents[in]  Also dump (some) contents?
*/
void pcb_queue_dump(const char *msg, pcb_queue_t queue, bool_t contents)
{
	// report on this queue
	cio_printf("%s: ", msg);
	if (queue == NULL) {
		cio_puts("NULL???\n");
		return;
	}

	// first, the basic data
	cio_printf("head %08x tail %08x", (uint32_t)queue->head,
			   (uint32_t)queue->tail);

	// next, how the queue is ordered
	cio_printf(" order %s\n",
			   queue->order >= N_ORDERINGS ? "????" : ord_str[queue->order]);

	// if there are members in the queue, dump the first few PIDs
	if (contents && queue->head != NULL) {
		cio_puts(" PIDs: ");
		pcb_t *tmp = queue->head;
		for (int i = 0; i < 5 && tmp != NULL; ++i, tmp = tmp->next) {
			cio_printf(" [%u]", tmp->pid);
		}

		if (tmp != NULL) {
			cio_puts(" ...");
		}

		cio_putchar('\n');
	}
}

/**
** ptable_dump(msg,all)
**
** dump the contents of the "active processes" table
**
** @param msg[in]  Optional message to print
** @param all[in]  Dump all or only part of the relevant data
*/
void ptable_dump(const char *msg, bool_t all)
{
	if (msg) {
		cio_puts(msg);
	}
	cio_putchar(' ');

	int used = 0;
	int empty = 0;

	register pcb_t *pcb = ptable;
	for (int i = 0; i < N_PROCS; ++i) {
		if (pcb->state == STATE_UNUSED) {
			// an empty slot
			++empty;

		} else {
			// a non-empty slot
			++used;

			// if not dumping everything, add commas if needed
			if (!all && used) {
				cio_putchar(',');
			}

			// report the table slot #
			cio_printf(" #%d:", i);

			// and dump the contents
			pcb_dump(NULL, pcb, all);
		}
	}

	// only need this if we're doing one-line output
	if (!all) {
		cio_putchar('\n');
	}

	// sanity check - make sure we saw the correct number of table slots
	if ((used + empty) != N_PROCS) {
		cio_printf("Table size %d, used %d + empty %d = %d???\n", N_PROCS, used,
				   empty, used + empty);
	}
}

/**
** Name:    ptable_dump_counts
**
** Prints basic information about the process table (number of
** entries, number with each process state, etc.).
*/
void ptable_dump_counts(void)
{
	uint_t nstate[N_STATES] = { 0 };
	uint_t unknown = 0;

	int n = 0;
	pcb_t *ptr = ptable;
	while (n < N_PROCS) {
		if (ptr->state < 0 || ptr->state >= N_STATES) {
			++unknown;
		} else {
			++nstate[ptr->state];
		}
		++n;
		++ptr;
	}

	cio_printf("Ptable: %u ***", unknown);
	for (n = 0; n < N_STATES; ++n) {
		if (nstate[n]) {
			cio_printf(" %u %s", nstate[n],
					   state_str[n] != NULL ? state_str[n] : "???");
		}
	}
	cio_putchar('\n');
}
