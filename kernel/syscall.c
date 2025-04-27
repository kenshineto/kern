#include <comus/cpu.h>
#include <comus/syscalls.h>
#include <comus/drivers/acpi.h>
#include <comus/drivers/gpu.h>
#include <comus/drivers/pit.h>
#include <comus/memory.h>
#include <comus/procs.h>
#include <stdint.h>

#define RET(type, name) type *name = (type *)(&current_pcb->regs->rax)
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

static int sys_poweroff(void)
{
	// TODO: we should probably
	// kill all user processes
	// and then sync the fs
	acpi_shutdown();
	return 1;
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

	vADDR = mem_mapaddr(current_pcb->memctx, pADDR, (void *)0x1000000000, len,
						F_PRESENT | F_WRITEABLE | F_UNPRIVILEGED);
	if (vADDR == NULL)
		return 1;

	width = gpu_dev->width;
	height = gpu_dev->height;
	bpp = gpu_dev->bit_depth;

	mem_ctx_switch(current_pcb->memctx);
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
	[SYS_exit] = sys_exit, [SYS_waitpid] = NULL,
	[SYS_fork] = NULL,	   [SYS_exec] = NULL,
	[SYS_open] = NULL,	   [SYS_close] = NULL,
	[SYS_read] = NULL,	   [SYS_write] = sys_write,
	[SYS_getpid] = NULL,   [SYS_getppid] = NULL,
	[SYS_gettime] = NULL,  [SYS_getprio] = NULL,
	[SYS_setprio] = NULL,  [SYS_kill] = NULL,
	[SYS_sleep] = NULL,	   [SYS_brk] = NULL,
	[SYS_sbrk] = NULL,	   [SYS_poweroff] = sys_poweroff,
	[SYS_drm] = sys_drm,   [SYS_ticks] = sys_ticks,
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
	current_pcb->regs->rax = 0;

	// syscall number

	// check for invalid syscall
	if (num >= N_SYSCALLS) {
		// invalid syscall
		// FIXME: kill user process
		while (1)
			;
	}

	// run syscall handler
	handler = syscall_tbl[num];
	if (handler != NULL)
		ret = handler();

	// on failure, set rax
	if (ret)
		current_pcb->regs->rax = ret;
}
