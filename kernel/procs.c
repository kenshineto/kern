#include <comus/procs.h>
#include <comus/error.h>
#include <comus/cpu.h>
#include <comus/memory.h>

#define PCB_QUEUE_EMPTY(q) ((q)->head == NULL)

struct pcb_queue_s {
	struct pcb *head;
	struct pcb *tail;
	enum pcb_queue_order order;
};

// collection of queues
static struct pcb_queue_s pcb_freelist_queue;
static struct pcb_queue_s ready_queue;
static struct pcb_queue_s waiting_queue;
static struct pcb_queue_s sleeping_queue;
static struct pcb_queue_s zombie_queue;

// public facing queue handels
pcb_queue_t pcb_freelist;
pcb_queue_t ready;
pcb_queue_t waiting;
pcb_queue_t sleeping;
pcb_queue_t zombie;

// pointer to the currently-running process
struct pcb *current;

/// pointer to the currently-running process
struct pcb *current;

/// the process table
struct pcb ptable[N_PROCS];

/// next avaliable pid
pid_t next_pid;

/// pointer to the pcb for the 'init' process
struct pcb *init_pcb;

/// table of state name strings
const char *proc_state_str[N_PROC_STATES] = {
	[PROC_STATE_UNUSED] = "Unu",   [PROC_STATE_NEW] = "New",
	[PROC_STATE_READY] = "Rdy",	   [PROC_STATE_RUNNING] = "Run",
	[PROC_STATE_SLEEPING] = "Slp", [PROC_STATE_BLOCKED] = "Blk",
	[PROC_STATE_WAITING] = "Wat",  [PROC_STATE_KILLED] = "Kil",
	[PROC_STATE_ZOMBIE] = "Zom",
};

/// table of priority name strings
const char *proc_prio_str[N_PROC_PRIOS] = {
	[PROC_PRIO_HIGH] = "High",
	[PROC_PRIO_STD] = "User",
	[PROC_PRIO_LOW] = "Low ",
	[PROC_PRIO_DEFERRED] = "Def ",
};

/// table of queue ordering name strings
const char *pcb_ord_str[N_PCB_ORDERINGS] = {
	[O_PCB_FIFO] = "FIFO",
	[O_PCB_PRIO] = "PRIO",
	[O_PCB_PID] = "PID ",
	[O_PCB_WAKEUP] = "WAKE",
};

/**
 * Priority search functions. These are used to traverse a supplied
 * queue looking for the queue entry that would precede the supplied
 * PCB when that PCB is inserted into the queue.
 *
 * Variations:
 *     find_prev_wakeup()     compares wakeup times
 *     find_prev_priority()   compares process priorities
 *     find_prev_pid()        compares PIDs
 *
 * Each assumes the queue should be in ascending order by the specified
 * comparison value.
 *
 * @param[in] queue  The queue to search
 * @param[in] pcb    The PCB to look for
 *
 * @return a pointer to the predecessor in the queue, or NULL if
 * this PCB would be at the beginning of the queue.
 */
static struct pcb *find_prev_wakeup(pcb_queue_t queue, struct pcb *pcb)
{
	// sanity checks!
	assert(queue != NULL, "find_prev_wakeup: queue is null");
	assert(pcb != NULL, "find_prev_wakeup: pcb is null");

	struct pcb *prev = NULL;
	struct pcb *curr = queue->head;

	while (curr != NULL && curr->wakeup <= pcb->wakeup) {
		prev = curr;
		curr = curr->next;
	}

	return prev;
}

static struct pcb *find_prev_priority(pcb_queue_t queue, struct pcb *pcb)
{
	// sanity checks!
	assert(queue != NULL, "find_prev_priority: queue is null");
	assert(pcb != NULL, "find_prev_priority: pcb is null");

	struct pcb *prev = NULL;
	struct pcb *curr = queue->head;

	while (curr != NULL && curr->priority <= pcb->priority) {
		prev = curr;
		curr = curr->next;
	}

	return prev;
}

static struct pcb *find_prev_pid(pcb_queue_t queue, struct pcb *pcb)
{
	// sanity checks!
	assert(queue != NULL, "find_prev_pid: queue is null");
	assert(pcb != NULL, "find_prev_pid: pcb is null");

	struct pcb *prev = NULL;
	struct pcb *curr = queue->head;

	while (curr != NULL && curr->pid <= pcb->pid) {
		prev = curr;
		curr = curr->next;
	}

	return prev;
}

// a macro to simplify queue setup
#define QINIT(q, s)                         \
	q = &q##_queue;                         \
	if (pcb_queue_reset(q, s) != SUCCESS) { \
		panic("pcb_init can't reset " #q);  \
	}

void pcb_init(void)
{
	// there is no current process
	current = NULL;

	// set up the external links to the queues
	QINIT(pcb_freelist, O_PCB_FIFO);
	QINIT(ready, O_PCB_PRIO);
	QINIT(waiting, O_PCB_PID);
	QINIT(sleeping, O_PCB_WAKEUP);
	QINIT(zombie, O_PCB_PID);

	// We statically allocate our PCBs, so we need to add them
	// to the freelist before we can use them. If this changes
	// so that we dynamically allocate PCBs, this step either
	// won't be required, or could be used to pre-allocate some
	// number of PCB structures for future use.

	struct pcb *ptr = ptable;
	for (int i = 0; i < N_PROCS; ++i) {
		pcb_free(ptr);
		++ptr;
	}
}

int pcb_alloc(struct pcb **pcb)
{
	// sanity check!
	assert(pcb != NULL, "pcb_alloc: allocating a non free pcb pointer");

	// remove the first PCB from the free list
	struct pcb *tmp;
	if (pcb_queue_pop(pcb_freelist, &tmp) != SUCCESS)
		return E_NO_PCBS;

	*pcb = tmp;
	return SUCCESS;
}

void pcb_free(struct pcb *pcb)
{
	if (pcb != NULL) {
		// mark the PCB as available
		pcb->state = PROC_STATE_UNUSED;

		// add it to the free list
		int status = pcb_queue_insert(pcb_freelist, pcb);

		// if that failed, we're in trouble
		if (status != SUCCESS) {
			panic("pcb_free(%16p) status %d", (void *)pcb, status);
		}
	}
}

void pcb_zombify(register struct pcb *victim)
{
	// should this be an error?
	if (victim == NULL)
		return;

	// every process must have a parent, even if it's 'init'
	assert(victim->parent != NULL, "pcb_zombify: process missing a parent");

	// We need to locate the parent of this process.  We also need
	// to reparent any children of this process.  We do these in
	// a single loop.
	struct pcb *parent = victim->parent;
	struct pcb *zchild = NULL;

	// two PIDs we will look for
	pid_t vicpid = victim->pid;

	// speed up access to the process table entries
	register struct pcb *curr = ptable;

	for (int i = 0; i < N_PROCS; ++i, ++curr) {
		// make sure this is a valid entry
		if (curr->state == PROC_STATE_UNUSED)
			continue;

		// if this is our parent, just keep going - we continue
		// iterating to find all the children of this process.
		if (curr == parent)
			continue;

		if (curr->parent == victim) {
			// found a child - reparent it
			curr->parent = init_pcb;

			// see if this child is already undead
			if (curr->state == PROC_STATE_ZOMBIE) {
				// if it's already a zombie, remember it, so we
				// can pass it on to 'init'; also, if there are
				// two or more zombie children, it doesn't matter
				// which one we pick here, as the others will be
				// collected when 'init' loops
				zchild = curr;
			}
		}
	}

	// If we found a child that was already terminated, we need to
	// wake up the init process if it's already waiting.
	//
	// NOTE: we only need to do this for one Zombie child process -
	// init will loop and collect the others after it finishes with
	// this one.
	//
	// Also note: it's possible that the exiting process' parent is
	// also init, which means we're letting one of zombie children
	// of the exiting process be cleaned up by init before the
	// existing process itself is cleaned up by init. This will work,
	// because after init cleans up the zombie, it will loop and
	// call waitpid() again, by which time this exiting process will
	// be marked as a zombie.

	if (zchild != NULL && init_pcb->state == PROC_STATE_WAITING) {
		// dequeue the zombie
		assert(pcb_queue_remove(zombie, zchild) == SUCCESS,
			   "pcb_zombify: cannot remove zombie process from queue");

		assert(pcb_queue_remove(waiting, init_pcb) == SUCCESS,
			   "pcb_zombify: cannot remove waiting process from queue");

		// intrinsic return value is the PID
		PCB_RET(init_pcb) = zchild->pid;

		// may also want to return the exit status
		int64_t *ptr = (int64_t *)PCB_ARG(init_pcb, 2);

		if (ptr != NULL) {
			// BUG:
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

	// Now, deal with the parent of this process. If the parent is
	// already waiting, just wake it up and clean up this process.
	// Otherwise, this process becomes a zombie.
	//
	// NOTE: if the exiting process' parent is init and we just woke
	// init up to deal with a zombie child of the exiting process,
	// init's status won't be Waiting any more, so we don't have to
	// worry about it being scheduled twice.

	if (parent->state == PROC_STATE_WAITING) {
		// verify that the parent is either waiting for this process
		// or is waiting for any of its children
		uint64_t target = PCB_ARG(parent, 1);

		if (target == 0 || target == vicpid) {
			// the parent is waiting for this child or is waiting
			// for any of its children, so we can wake it up.

			// intrinsic return value is the PID
			PCB_RET(parent) = vicpid;

			// may also want to return the exit status
			int64_t *ptr = (int64_t *)PCB_ARG(parent, 2);

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

	// The parent isn't waiting OR is waiting for a specific child
	// that isn't this exiting process, so we become a Zombie.
	//
	// This code assumes that Zombie processes are *not* in
	// a queue, but instead are just in the process table with
	// a state of 'Zombie'.  This simplifies life immensely,
	// because we won't need to dequeue it when it is collected
	// by its parent.

	victim->state = PROC_STATE_ZOMBIE;
	assert(pcb_queue_insert(zombie, victim) == SUCCESS,
		   "cannot insert victim process into zombie queue");

	// Note: we don't call _dispatch() here - we leave that for
	// the calling routine, as it's possible we don't need to
	// choose a new current process.
}

void pcb_cleanup(struct pcb *pcb)
{
	// avoid deallocating a NULL pointer
	if (pcb == NULL) {
		// should this be an error?
		return;
	}

	// we need to release all the VM data structures and frames
	mem_ctx_free(pcb->memctx);
	// TODO: close open files

	// release the PCB itself
	pcb_free(pcb);
}

struct pcb *pcb_find_pid(pid_t pid)
{
	// must be a valid PID
	if (pid < 1) {
		return NULL;
	}

	// scan the process table
	struct pcb *p = ptable;

	for (int i = 0; i < N_PROCS; ++i, ++p) {
		if (p->pid == pid && p->state != PROC_STATE_UNUSED) {
			return p;
		}
	}

	// didn't find it!
	return NULL;
}

struct pcb *pcb_find_ppid(pid_t pid)
{
	// must be a valid PID
	if (pid < 1) {
		return NULL;
	}

	// scan the process table
	struct pcb *p = ptable;

	for (int i = 0; i < N_PROCS; ++i, ++p) {
		assert(p->parent != NULL,
			   "pcb_find_ppid: process does not have a parent");
		if (p->parent->pid == pid && p->parent->state != PROC_STATE_UNUSED) {
			return p;
		}
	}

	// didn't find it!
	return NULL;
}

int pcb_queue_reset(pcb_queue_t queue, enum pcb_queue_order style)
{
	// sanity check
	assert(queue != NULL, "pcb_queue_reset: queue is null");

	// make sure the style is valid
	if (style < 0 || style >= N_PCB_ORDERINGS) {
		return E_BAD_PARAM;
	}

	// reset the queue
	queue->head = queue->tail = NULL;
	queue->order = style;

	return SUCCESS;
}

bool pcb_queue_empty(pcb_queue_t queue)
{
	// if there is no queue, blow up
	assert(queue != NULL, "pcb_queue_empty: queue is empty");

	return PCB_QUEUE_EMPTY(queue);
}

size_t pcb_queue_length(const pcb_queue_t queue)
{
	// sanity check
	assert(queue != NULL, "pcb_queue_length: queue is null");

	// this is pretty simple
	register struct pcb *tmp = queue->head;
	register size_t num = 0;

	while (tmp != NULL) {
		++num;
		tmp = tmp->next;
	}

	return num;
}

int pcb_queue_insert(pcb_queue_t queue, struct pcb *pcb)
{
	// sanity checks
	assert(queue != NULL, "pcb_queue_insert: queue is null");
	assert(pcb != NULL, "pcb_queue_insert: pcb is null");

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
	assert(queue->tail != NULL, "pcb_queue_insert: queue tail is null");

	// no, so we need to search it
	struct pcb *prev = NULL;

	// find the predecessor node
	switch (queue->order) {
	case O_PCB_FIFO:
		prev = queue->tail;
		break;
	case O_PCB_PRIO:
		prev = find_prev_priority(queue, pcb);
		break;
	case O_PCB_PID:
		prev = find_prev_pid(queue, pcb);
		break;
	case O_PCB_WAKEUP:
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

int pcb_queue_pop(pcb_queue_t queue, struct pcb **pcb)
{
	//sanity checks
	assert(queue != NULL, "pcb_queue_pop: queue is null");
	assert(pcb != NULL, "pcb_queue_pop: pcb is null");

	// can't get anything if there's nothing to get!
	if (PCB_QUEUE_EMPTY(queue)) {
		return E_EMPTY_QUEUE;
	}

	// take the first entry from the queue
	struct pcb *tmp = queue->head;
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

int pcb_queue_remove(pcb_queue_t queue, struct pcb *pcb)
{
	//sanity checks
	assert(queue != NULL, "pcb_queue_remove: queue is null");
	assert(pcb != NULL, "pcb_queue_remove: pcb is null");

	// can't get anything if there's nothing to get!
	if (PCB_QUEUE_EMPTY(queue)) {
		return E_EMPTY_QUEUE;
	}

	// iterate through the queue until we find the desired PCB
	struct pcb *prev = NULL;
	struct pcb *curr = queue->head;

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
		assert(prev != NULL, "pcb_queue_remove: prev element in queue is null");
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

	assert((queue->head == NULL && queue->tail == NULL) ||
			   (queue->head != NULL && queue->tail != NULL),
		   "pcb_queue_remove: queue consistancy problem");

	return SUCCESS;
}

struct pcb *pcb_queue_peek(const pcb_queue_t queue)
{
	//sanity check
	assert(queue != NULL, "pcb_queue_peek: queue is null");

	// can't get anything if there's nothing to get!
	if (PCB_QUEUE_EMPTY(queue)) {
		return NULL;
	}

	// just return the first entry from the queue
	return queue->head;
}

void schedule(struct pcb *pcb)
{
	// sanity check
	assert(pcb != NULL, "schedule: pcb is null");

	// check for a killed process
	if (pcb->state == PROC_STATE_KILLED)
		panic("attempted to schedule killed process %d", pcb->pid);

	// mark it as ready
	pcb->state = PROC_STATE_READY;

	// add it to the ready queue
	if (pcb_queue_insert(ready, pcb) != SUCCESS)
		panic("schedule insert fail");
}

void dispatch(void)
{
	// verify that there is no current process
	assert(current == NULL, "dispatch: current process is not null");

	// grab whoever is at the head of the queue
	int status = pcb_queue_pop(ready, &current);
	if (status != SUCCESS) {
		panic("dispatch queue remove failed, code %d", status);
	}

	// set the process up for success
	current->state = PROC_STATE_RUNNING;
	current->ticks = PROC_QUANTUM_STANDARD;
}

void ctx_dump(const char *msg, register struct cpu_regs *regs)
{
	// first, the message (if there is one)
	if (msg)
		kputs(msg);

	// the pointer
	kprintf(" @ %16p: ", (void *)regs);

	// if it's NULL, why did you bother calling me?
	if (regs == NULL) {
		kprintf(" NULL???\n");
		return;
	}

	// now, the contents
	kputc('\n');
	cpu_print_regs(regs);
}

void ctx_dump_all(const char *msg)
{
	// first, the message (if there is one)
	if (msg)
		kputs(msg);

	int n = 0;
	register struct pcb *pcb = ptable;
	for (int i = 0; i < N_PROCS; ++i, ++pcb) {
		if (pcb->state != PROC_STATE_UNUSED) {
			++n;
			kprintf("%2d(%d): ", n, pcb->pid);
			ctx_dump(NULL, pcb->regs);
		}
	}
}

void pcb_dump(const char *msg, register struct pcb *pcb, bool all)
{
	// first, the message (if there is one)
	if (msg)
		kputs(msg);

	// the pointer
	kprintf(" @ %16px:", (void *)pcb);

	// if it's NULL, why did you bother calling me?
	if (pcb == NULL) {
		kputs(" NULL???\n");
		return;
	}

	kprintf(" %d", pcb->pid);
	kprintf(" %s",
			pcb->state >= N_PROC_STATES ? "???" : proc_state_str[pcb->state]);

	if (!all) {
		// just printing IDs and states on one line
		return;
	}

	// now, the rest of the contents
	kprintf(" %s", pcb->priority >= N_PROC_PRIOS ?
					   "???" :
					   proc_prio_str[pcb->priority]);

	kprintf(" ticks %zu xit %d wake %16lx\n", pcb->ticks, pcb->exit_status,
			pcb->wakeup);

	kprintf(" parent %16p", (void *)pcb->regs);
	if (pcb->parent != NULL) {
		kprintf(" (%u)", pcb->parent->pid);
	}

	kprintf(" next %16p context %16p memctx %16p", (void *)pcb->next,
			(void *)pcb->regs, (void *)pcb->memctx);

	kputc('\n');
}

void pcb_queue_dump(const char *msg, pcb_queue_t queue, bool contents)
{
	// report on this queue
	kprintf("%s: ", msg);
	if (queue == NULL) {
		kputs("NULL???\n");
		return;
	}

	// first, the basic data
	kprintf("head %16p tail %16p", (void *)queue->head, (void *)queue->tail);

	// next, how the queue is ordered
	kprintf(" order %s\n", queue->order >= N_PCB_ORDERINGS ?
							   "????" :
							   pcb_ord_str[queue->order]);

	// if there are members in the queue, dump the first few PIDs
	if (contents && queue->head != NULL) {
		kputs(" PIDs: ");
		struct pcb *tmp = queue->head;
		for (int i = 0; i < 5 && tmp != NULL; ++i, tmp = tmp->next) {
			kprintf(" [%u]", tmp->pid);
		}

		if (tmp != NULL) {
			kputs(" ...");
		}

		kputc('\n');
	}
}

void ptable_dump(const char *msg, bool all)
{
	if (msg)
		kputs(msg);
	kputc(' ');

	int used = 0;
	int empty = 0;

	register struct pcb *pcb = ptable;
	for (int i = 0; i < N_PROCS; ++i) {
		if (pcb->state == PROC_STATE_UNUSED) {
			// an empty slot
			++empty;

		} else {
			// a non-empty slot
			++used;

			// if not dumping everything, add commas if needed
			if (!all && used) {
				kputc(',');
			}

			// report the table slot #
			kprintf(" #%d:", i);

			// and dump the contents
			pcb_dump(NULL, pcb, all);
		}
	}

	// only need this if we're doing one-line output
	if (!all) {
		kputc('\n');
	}

	// sanity check - make sure we saw the correct number of table slots
	if ((used + empty) != N_PROCS) {
		kprintf("Table size %d, used %d + empty %d = %d???\n", N_PROCS, used,
				empty, used + empty);
	}
}

void ptable_dump_counts(void)
{
	size_t nstate[N_PROC_STATES] = { 0 };
	size_t unknown = 0;

	int n = 0;
	struct pcb *ptr = ptable;
	while (n < N_PROCS) {
		if (ptr->state < 0 || ptr->state >= N_PROC_STATES) {
			++unknown;
		} else {
			++nstate[ptr->state];
		}
		++n;
		++ptr;
	}

	kprintf("Ptable: %zu ***", unknown);
	for (n = 0; n < N_PROC_STATES; ++n) {
		kprintf(" %zu %s", nstate[n],
				proc_state_str[n] != NULL ? proc_state_str[n] : "???");
	}
	kputc('\n');
}
