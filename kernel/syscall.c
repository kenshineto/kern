#include <comus/cpu.h>
#include <comus/syscalls.h>
#include <comus/drivers/uart.h>
#include <comus/memory.h>
#include <comus/procs.h>

#define ARG1(type, name) type name = (type)(current_pcb->regs->rdi)
#define ARG2(type, name) type name = (type)(current_pcb->regs->rsi)
#define ARG3(type, name) type name = (type)(current_pcb->regs->rdx)
#define ARG4(type, name) type name = (type)(current_pcb->regs->rcx)

static int sys_exit(void)
{
	ARG1(int, status);
	(void)status;

	// FIXME: schedule somthing else
	while (1)
		;

	return 1;
}

static int sys_write(void)
{
	ARG1(int, fd);
	ARG2(const void *, buffer);
	ARG3(size_t, nbytes);

	const char *map_buf = kmapuseraddr(current_pcb->memctx, buffer, nbytes);
	if (map_buf == NULL)
		return 0;

	// cannot write to stdin
	if (fd == 0)
		nbytes = 0;

	// write to stdout
	else if (fd == 1) {
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

static int (*syscall_tbl[N_SYSCALLS])(void) = {
	[SYS_exit] = sys_exit, [SYS_waitpid] = NULL,	[SYS_fork] = NULL,
	[SYS_exec] = NULL,	   [SYS_open] = NULL,		[SYS_close] = NULL,
	[SYS_read] = NULL,	   [SYS_write] = sys_write, [SYS_getpid] = NULL,
	[SYS_getppid] = NULL,  [SYS_gettime] = NULL,	[SYS_getprio] = NULL,
	[SYS_setprio] = NULL,  [SYS_kill] = NULL,		[SYS_sleep] = NULL,
	[SYS_brk] = NULL,	   [SYS_sbrk] = NULL,
};

void syscall_handler(struct cpu_regs *regs)
{
	uint64_t num;
	int (*handler)(void);
	int ret = 1;

	// make sure were in the kernel memory context
	mem_ctx_switch(kernel_mem_ctx);

	// update data
	current_pcb->regs = regs;
	num = current_pcb->regs->rax;

	// syscall number

	// check for invalid syscall
	if (num >= N_SYSCALLS) {
		// invalid syscall
		// FIXME: kill user process
		while (1)
			;
	}

	// run syscall handler (if exists)
	handler = syscall_tbl[num];
	if (handler != NULL)
		handler();

	// save return value
	current_pcb->regs->rax = ret;

	// switch back to process ctx
	mem_ctx_switch(current_pcb->memctx);
}
