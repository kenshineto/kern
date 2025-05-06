#include <comus/drivers/pit.h>
#include <comus/syscalls.h>
#include <comus/memory.h>
#include <comus/procs.h>
#include <comus/error.h>
#include <comus/cpu.h>
#include <comus/asm.h>

#define PCB_QUEUE_EMPTY(q) ((q)->head == NULL)

struct pcb_queue_s {
	struct pcb *head;
	struct pcb *tail;
	enum pcb_queue_order order;
};

// collection of queues
static struct pcb_queue_s _pcb_freelist;
static struct pcb_queue_s _ready_queue;
static struct pcb_queue_s _zombie_queue;
static struct pcb_queue_s _syscall_queue[N_SYSCALLS];

// public facing queue handels
pcb_queue_t pcb_freelist;
pcb_queue_t ready_queue;
pcb_queue_t zombie_queue;
pcb_queue_t syscall_queue[N_SYSCALLS];

/// pointer to the currently-running process
struct pcb *current_pcb = NULL;

/// pointer to the pcb for the 'init' process
struct pcb *init_pcb = NULL;

/// the process table
struct pcb ptable[N_PROCS];

/// next avaliable pid
pid_t next_pid = 1;

static struct pcb *find_prev_wakeup(pcb_queue_t queue, struct pcb *pcb)
{
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
	q = &_##q;                              \
	if (pcb_queue_reset(q, s) != SUCCESS) { \
		panic("pcb_init can't reset " #q);  \
	}

void pcb_init(void)
{
	// there is no current process
	current_pcb = NULL;

	// set up the external links to the queues
	QINIT(pcb_freelist, O_PCB_FIFO);
	QINIT(ready_queue, O_PCB_PRIO);
	QINIT(zombie_queue, O_PCB_PID);
	for (size_t i = 0; i < N_SYSCALLS; i++) {
		QINIT(syscall_queue[i], O_PCB_PID);
	}

	// setup pcb linked list (free list)
	// this can be done by calling pcb_free :)
	struct pcb *ptr = ptable;
	for (int i = 0; i < N_PROCS; ++i) {
		pcb_free(ptr);
		++ptr;
	}
}

int pcb_alloc(struct pcb **pcb)
{
	assert(pcb != NULL, "pcb_alloc: allocating a non free pcb pointer");

	// remove the first PCB from the free list
	struct pcb *tmp;
	if (pcb_queue_pop(pcb_freelist, &tmp) != SUCCESS)
		return E_NO_PCBS;

	tmp->pid = next_pid++;
	tmp->state = PROC_STATE_NEW;
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

void pcb_zombify(struct pcb *victim)
{
	if (victim == NULL)
		return;

	assert(victim->pid != 1,
		   "pcb_zombify: attemped to zombie the init process");
	assert(victim->parent != NULL, "pcb_zombify: process missing a parent");

	struct pcb *parent = victim->parent;
	struct pcb *zchild = NULL;
	struct pcb *curr = ptable;

	// find all children of victim and reparent
	for (int i = 0; i < N_PROCS; ++i, ++curr) {
		// is this a valid entry?
		if (curr->state == PROC_STATE_UNUSED)
			continue;

		// is this not our child?
		if (curr->parent != victim)
			continue;

		// reparent to init
		curr->parent = init_pcb;

		// if this child is a zombie
		if (curr->state == PROC_STATE_ZOMBIE) {
			// if it's already a zombie, remember it, so we
			// can pass it on to 'init'; also, if there are
			// two or more zombie children, it doesn't matter
			// which one we pick here, as the others will be
			// collected when 'init' loops
			zchild = curr;
		}
	}

	// schedule init if zombie child found
	if (zchild != NULL && init_pcb->syscall == SYS_waitpid) {
		pid_t pid;
		int *status;

		assert(pcb_queue_remove(zombie_queue, zchild) == SUCCESS,
			   "pcb_zombify: cannot remove zombie process from queue");
		assert(pcb_queue_remove(syscall_queue[SYS_waitpid], init_pcb) ==
				   SUCCESS,
			   "pcb_zombify: cannot remove waiting process from queue");

		pid = (pid_t)PCB_ARG1(init_pcb);
		status = (int *)PCB_ARG2(init_pcb);

		// set exited pid and exist status in init's waitpid call
		if (pid == 0 || pid == zchild->pid) {
			PCB_RET(init_pcb) = zchild->pid;
			if (status != NULL) {
				mem_ctx_switch(init_pcb->memctx);
				*status = zchild->exit_status;
				mem_ctx_switch(kernel_mem_ctx);
			}
			schedule(init_pcb);
		}

		pcb_cleanup(zchild);
	}

	// if the parent is waiting, wake it up and clean the victim,
	// otherwise the victim will become a zombie
	if (parent->syscall == SYS_waitpid) {
		pid_t pid;
		int *status;

		pid = (pid_t)PCB_ARG1(parent);
		status = (int *)PCB_ARG2(parent);

		if (pid == 0 || pid == victim->pid) {
			PCB_RET(parent) = victim->pid;

			assert(
				pcb_queue_remove(syscall_queue[SYS_waitpid], parent) == SUCCESS,
				"pcb_zombify: cannot remove parent process from waitpid queue");

			if (status != NULL) {
				mem_ctx_switch(parent->memctx);
				*status = victim->exit_status;
				mem_ctx_switch(kernel_mem_ctx);
			}
			schedule(parent);
			pcb_cleanup(victim);
			return;
		}
	}

	victim->state = PROC_STATE_ZOMBIE;
	assert(pcb_queue_insert(zombie_queue, victim) == SUCCESS,
		   "cannot insert victim process into zombie queue");
}

void pcb_cleanup(struct pcb *pcb)
{
	if (pcb == NULL)
		return;
	if (pcb->memctx)
		mem_ctx_free(pcb->memctx);
	pcb_free(pcb);
}

struct pcb *pcb_find_pid(pid_t pid)
{
	if (pid < 1)
		return NULL;

	struct pcb *p = ptable;
	for (int i = 0; i < N_PROCS; ++i, ++p) {
		if (p->pid == pid && p->state != PROC_STATE_UNUSED) {
			return p;
		}
	}

	return NULL;
}

struct pcb *pcb_find_ppid(pid_t pid)
{
	if (pid < 1)
		return NULL;

	struct pcb *p = ptable;
	for (int i = 0; i < N_PROCS; ++i, ++p) {
		assert(p->parent != NULL,
			   "pcb_find_ppid: process does not have a parent");
		if (p->parent->pid == pid && p->parent->state != PROC_STATE_UNUSED) {
			return p;
		}
	}

	return NULL;
}

int pcb_queue_reset(pcb_queue_t queue, enum pcb_queue_order style)
{
	assert(queue != NULL, "pcb_queue_reset: queue is null");

	if (style < 0 || style >= N_PCB_ORDERINGS)
		return E_BAD_PARAM;

	queue->head = queue->tail = NULL;
	queue->order = style;
	return SUCCESS;
}

bool pcb_queue_empty(pcb_queue_t queue)
{
	assert(queue != NULL, "pcb_queue_empty: queue is empty");
	return PCB_QUEUE_EMPTY(queue);
}

size_t pcb_queue_length(const pcb_queue_t queue)
{
	assert(queue != NULL, "pcb_queue_length: queue is null");

	struct pcb *tmp = queue->head;
	size_t num = 0;
	while (tmp != NULL) {
		++num;
		tmp = tmp->next;
	}

	return num;
}

int pcb_queue_insert(pcb_queue_t queue, struct pcb *pcb)
{
	assert(queue != NULL, "pcb_queue_insert: queue is null");
	assert(pcb != NULL, "pcb_queue_insert: pcb is null");

	if (pcb->next != NULL)
		return E_BAD_PARAM;

	if (queue->head == NULL) {
		queue->head = queue->tail = pcb;
		return SUCCESS;
	}
	assert(queue->tail != NULL, "pcb_queue_insert: queue tail is null");

	struct pcb *prev = NULL;
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
		return E_BAD_PARAM;
	}

	// found the predecessor node, time to do the insertion
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
	assert(queue != NULL, "pcb_queue_pop: queue is null");
	assert(pcb != NULL, "pcb_queue_pop: pcb is null");

	if (PCB_QUEUE_EMPTY(queue))
		return E_EMPTY_QUEUE;

	struct pcb *tmp = queue->head;
	queue->head = tmp->next;
	tmp->next = NULL;
	if (queue->head == NULL) {
		queue->tail = NULL;
	}

	*pcb = tmp;
	return SUCCESS;
}

int pcb_queue_remove(pcb_queue_t queue, struct pcb *pcb)
{
	assert(queue != NULL, "pcb_queue_remove: queue is null");
	assert(pcb != NULL, "pcb_queue_remove: pcb is null");

	if (PCB_QUEUE_EMPTY(queue))
		return E_EMPTY_QUEUE;

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
	assert(queue != NULL, "pcb_queue_peek: queue is null");

	if (PCB_QUEUE_EMPTY(queue))
		return NULL;

	return queue->head;
}

void schedule(struct pcb *pcb)
{
	assert(pcb != NULL, "schedule: pcb is null");
	assert(pcb->state != PROC_STATE_UNUSED,
		   "attempted to schedule invalid process %d", pcb->pid);
	assert(pcb->state != PROC_STATE_ZOMBIE,
		   "attempted to schedule killed process %d", pcb->pid);

	pcb->state = PROC_STATE_READY;
	pcb->syscall = 0;

	if (pcb_queue_insert(ready_queue, pcb) != SUCCESS)
		panic("schedule insert fail");
}

__attribute__((noreturn)) void dispatch(void)
{
	int status;

	assert(current_pcb == NULL, "dispatch: current process is not null");

	// wait for a process to schedule
	do {
		status = pcb_queue_pop(ready_queue, &current_pcb);
		if (status == SUCCESS) {
			break;
		}
		int_wait();
	} while (1);

	// set the process up for success
	current_pcb->regs.cr3 = (uint64_t)mem_ctx_pgdir(current_pcb->memctx);
	current_pcb->state = PROC_STATE_RUNNING;
	current_pcb->ticks = 3; // ticks per process
	current_pcb->syscall = 0;

	syscall_return();
}

void pcb_on_tick(void)
{
	// procs not initalized yet
	if (init_pcb == NULL)
		return;

	// update on sleeping
	do {
		struct pcb *pcb;

		if (pcb_queue_empty(syscall_queue[SYS_sleep]))
			break;

		pcb = pcb_queue_peek(syscall_queue[SYS_sleep]);
		assert(pcb != NULL, "sleeping queue should not be empty");

		if (pcb->wakeup >= ticks)
			break;

		if (pcb_queue_remove(syscall_queue[SYS_sleep], pcb))
			panic("failed to wake sleeping process: %d", pcb->pid);

		schedule(pcb);
	} while (1);

	if (current_pcb) {
		current_pcb->ticks--;
		if (current_pcb->ticks < 1) {
			// schedule another process
			schedule(current_pcb);
			current_pcb = NULL;
			dispatch();
		}
	}
}
