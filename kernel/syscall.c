#include <comus/cpu.h>
#include <comus/syscalls.h>
#include <comus/drivers/acpi.h>
#include <comus/drivers/gpu.h>
#include <comus/drivers/pit.h>
#include <comus/memory.h>
#include <comus/procs.h>
#include <comus/time.h>
#include <lib.h>

static struct pcb *pcb;

#define RET(type, name) type *name = (type *)(&pcb->regs.rax)
#define ARG1(type, name) type name = (type)(pcb->regs.rdi)
#define ARG2(type, name) type name = (type)(pcb->regs.rsi)
#define ARG3(type, name) type name = (type)(pcb->regs.rdx)
#define ARG4(type, name) type name = (type)(pcb->regs.rcx)

#define stdin 0
#define stdout 1
#define stderr 2

__attribute__((noreturn)) static int sys_exit(void)
{
	ARG1(int, status);

	pcb->exit_status = status;
	pcb_zombify(pcb);

	// call next process
	dispatch();
}

static int sys_waitpid(void)
{
	// arguments are read later
	// by procs.c
	pcb->state = PROC_STATE_WAITING;
	pcb_queue_insert(waiting, pcb);

	// call next process
	dispatch();
}

static int sys_write(void)
{
	ARG1(int, fd);
	ARG2(const void *, buffer);
	ARG3(size_t, nbytes);

	const char *map_buf = kmapuseraddr(pcb->memctx, buffer, nbytes);
	if (map_buf == NULL)
		return 0;

	// cannot write to stdin
	if (fd == 0)
		nbytes = 0;

	// write to stdout / stderr
	else if (fd == stdout || fd == stderr) {
		for (size_t i = 0; i < nbytes; i++)
			kputc(map_buf[i]);
	}

	// files
	else {
		// TODO: write to files
		nbytes = 0;
	}

	kunmapaddr(map_buf);

	return nbytes;
}

static int sys_getpid(void)
{
	return pcb->pid;
}

static int sys_getppid(void)
{
	// init's parent is itself
	if (pcb->parent == NULL)
		return init_pcb->pid;
	return pcb->parent->pid;
}

static int sys_gettime(void)
{
	RET(unsigned long, time);
	*time = unixtime();
	return 0;
}

static int sys_getprio(void)
{
	RET(unsigned int, prio);
	*prio = pcb->priority;
	return 0;
}

static int sys_setprio(void)
{
	RET(unsigned int, old);
	ARG1(unsigned int, new);

	*old = pcb->priority;
	pcb->priority = new;
	return 0;
}

static int sys_kill(void)
{
	ARG1(pid_t, pid);
	struct pcb *victim, *parent;

	victim = pcb_find_pid(pid);

	// pid does not exist
	if (victim == NULL)
		return 1;

	parent = victim;
	while (parent) {
		if (parent->pid == pcb->pid)
			break;
		parent = parent->parent;
	}

	// we do not own this pid
	if (parent == NULL)
		return 1;

	switch (victim->state) {
	case PROC_STATE_KILLED:
	case PROC_STATE_ZOMBIE:
		// you can't kill it if it's already dead
		return 0;

	case PROC_STATE_READY:
	case PROC_STATE_SLEEPING:
	case PROC_STATE_BLOCKED:
		// here, the process is on a queue somewhere; mark
		// it as "killed", and let the scheduler deal with it
		victim->state = PROC_STATE_KILLED;
		return 0;

	case PROC_STATE_RUNNING:
		// we have met the enemy, and it is us!
		pcb->exit_status = 1;
		pcb_zombify(pcb);
		// we need a new current process
		dispatch();
		break;

	case PROC_STATE_WAITING:
		// similar to the 'running' state, but we don't need
		// to dispatch a new process
		victim->exit_status = 1;
		pcb_queue_remove(waiting, pcb);
		pcb_zombify(victim);
		break;

	default:
		// cannot kill a previable process
		return 1;
	}

	return 0;
}

static int sys_sleep(void)
{
	RET(int, ret);
	ARG1(unsigned long, ms);

	if (ms == 0) {
		*ret = 0;
		schedule(pcb);
		dispatch();
	}

	pcb->wakeup = ticks + ms;
	if (pcb_queue_insert(sleeping, pcb)) {
		WARN("sleep pcb insert failed");
		return 1;
	}

	// calling pcb is in sleeping queue,
	// we must call a new one
	dispatch();
}

__attribute__((noreturn)) static int sys_poweroff(void)
{
	// TODO: we should probably
	// kill all user processes
	// and then sync the fs
	acpi_shutdown();
}

static int sys_drm(void)
{
	ARG1(void **, res_fb);
	ARG2(int *, res_width);
	ARG3(int *, res_height);
	ARG4(int *, res_bpp);

	void *pADDR, *vADDR;
	int width, height, bpp;
	size_t len;

	if (gpu_dev == NULL)
		return 1;

	len = gpu_dev->width * gpu_dev->height * gpu_dev->bit_depth / 8;
	pADDR =
		mem_get_phys(kernel_mem_ctx, (void *)(uintptr_t)gpu_dev->framebuffer);
	if (pADDR == NULL)
		return 1;

	vADDR = mem_mapaddr(pcb->memctx, pADDR, (void *)0x1000000000, len,
						F_PRESENT | F_WRITEABLE | F_UNPRIVILEGED);
	if (vADDR == NULL)
		return 1;

	width = gpu_dev->width;
	height = gpu_dev->height;
	bpp = gpu_dev->bit_depth;

	mem_ctx_switch(pcb->memctx);
	*res_fb = vADDR;
	*res_width = width;
	*res_height = height;
	*res_bpp = bpp;
	mem_ctx_switch(kernel_mem_ctx);

	return 0;
}

static int sys_ticks(void)
{
	RET(uint64_t, res_ticks);
	*res_ticks = ticks;
	return 0;
}

static int (*syscall_tbl[N_SYSCALLS])(void) = {
	[SYS_exit] = sys_exit,		 [SYS_waitpid] = sys_waitpid,
	[SYS_fork] = NULL,			 [SYS_exec] = NULL,
	[SYS_open] = NULL,			 [SYS_close] = NULL,
	[SYS_read] = NULL,			 [SYS_write] = sys_write,
	[SYS_getpid] = sys_getpid,	 [SYS_getppid] = sys_getppid,
	[SYS_gettime] = sys_gettime, [SYS_getprio] = sys_getprio,
	[SYS_setprio] = sys_setprio, [SYS_kill] = sys_kill,
	[SYS_sleep] = sys_sleep,	 [SYS_brk] = NULL,
	[SYS_sbrk] = NULL,			 [SYS_poweroff] = sys_poweroff,
	[SYS_drm] = sys_drm,		 [SYS_ticks] = sys_ticks,
};

void syscall_handler(void)
{
	uint64_t num;
	int (*handler)(void);
	int ret = 1;

	// update data
	pcb = current_pcb;
	num = pcb->regs.rax;
	pcb->regs.rax = 0;
	current_pcb = NULL;

	// check for invalid syscall
	if (num >= N_SYSCALLS) {
		// kill process
		pcb->exit_status = 1;
		pcb_zombify(pcb);
		// call next process
		dispatch();
	}

	// run syscall handler
	handler = syscall_tbl[num];
	if (handler != NULL)
		ret = handler();

	// on failure, set rax
	if (ret)
		pcb->regs.rax = ret;

	// return to current pcb
	current_pcb = pcb;
}
