#include "comus/fs.h"
#include "lib/kio.h"
#include <comus/user.h>
#include <comus/cpu.h>
#include <comus/syscalls.h>
#include <comus/drivers/acpi.h>
#include <comus/drivers/gpu.h>
#include <comus/drivers/pit.h>
#include <comus/memory.h>
#include <comus/procs.h>
#include <comus/time.h>
#include <comus/error.h>
#include <lib.h>
#include <stddef.h>

static struct pcb *pcb;

#define RET(type, name) type *name = (type *)(&pcb->regs.rax)
#define ARG1(type, name) type name = (type)(pcb->regs.rdi)
#define ARG2(type, name) type name = (type)(pcb->regs.rsi)
#define ARG3(type, name) type name = (type)(pcb->regs.rdx)
#define ARG4(type, name) type name = (type)(pcb->regs.rcx)

#define stdin 0
#define stdout 1
#define stderr 2

static struct file *get_file_ptr(int fd)
{
	// valid index?
	if (fd < 3 || fd >= (N_OPEN_FILES + 3))
		return NULL;

	// will be NULL if not open
	return pcb->open_files[fd - 3];
}

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
	ARG1(pid_t, pid);
	ARG2(int *, status);

	struct pcb *child;
	for (int i = 0; i < N_PROCS; i++) {
		child = &ptable[i];
		if (child->state != PROC_STATE_ZOMBIE)
			continue;
		if (child->parent != pcb)
			continue;

		// we found a child!
		if (pid && pid != child->pid)
			continue;

		// set status
		mem_ctx_switch(pcb->memctx);
		*status = child->exit_status;
		mem_ctx_switch(kernel_mem_ctx);

		// clean up child process
		pcb_cleanup(child);

		// return
		return child->pid;
	}

	// arguments are read later
	// by procs.c
	pcb->state = PROC_STATE_BLOCKED;
	assert(pcb_queue_insert(syscall_queue[SYS_waitpid], pcb) == SUCCESS,
		   "sys_waitpid: could not add process to waitpid queue");

	// call next process
	dispatch();
}

static int sys_fork(void)
{
	struct pcb *child;

	child = user_clone(pcb);
	if (child == NULL)
		return -1;

	child->regs.rax = 0;
	pcb->regs.rax = child->pid;
	schedule(child);
	return 0;
}

static int sys_exec(void)
{
	ARG1(const char *, in_filename);
	ARG2(const char **, in_args);

	struct file_system *fs;
	struct file *file;
	char filename[N_FILE_NAME];
	struct pcb save;

	// save data
	file = NULL;
	save = *pcb;

	// read filename
	mem_ctx_switch(pcb->memctx);
	memcpy(filename, in_filename, strlen(in_filename) + 1);
	mem_ctx_switch(kernel_mem_ctx);

	// get binary
	fs = fs_get_root_file_system();
	if (fs == NULL)
		goto fail;
	if (fs->open(fs, filename, O_RDONLY, &file))
		goto fail;

	// load program
	save = *pcb;
	if (user_load(pcb, file, in_args, save.memctx))
		goto fail;
	file->close(file);
	mem_ctx_free(save.memctx);
	schedule(pcb);
	dispatch();

fail:
	*pcb = save;
	if (file)
		file->close(file);
	return 1;
}

static int sys_open(void)
{
	ARG1(const char *, in_filename);
	ARG2(int, flags);

	char filename[N_FILE_NAME];
	struct file_system *fs;
	struct file **file;
	int fd;

	// read filename
	mem_ctx_switch(pcb->memctx);
	memcpy(filename, in_filename, strlen(in_filename));
	mem_ctx_switch(kernel_mem_ctx);

	// get fd
	for (fd = 3; fd < (N_OPEN_FILES + 3); fd++) {
		if (pcb->open_files[fd - 3] == NULL) {
			file = &pcb->open_files[fd - 3];
			break;
		}
	}

	// could not find fd
	if (fd == (N_OPEN_FILES + 3))
		return -1;

	// open file
	fs = fs_get_root_file_system();
	if (fs == NULL)
		return -1;
	if (fs->open(fs, filename, flags, file))
		return -1;

	// file opened
	return fd;
}

static int sys_close(void)
{
	ARG1(int, fd);

	struct file *file;
	file = get_file_ptr(fd);
	if (file == NULL)
		return 1;

	file->close(file);
	return 0;
}

static int sys_read(void)
{
	ARG1(int, fd);
	ARG2(void *, buffer);
	ARG3(size_t, nbytes);

	struct file *file;
	char *map_buf;

	map_buf = kmapuseraddr(pcb->memctx, buffer, nbytes);
	if (map_buf == NULL)
		return -1;

	file = get_file_ptr(fd);
	if (file == NULL)
		goto fail;

	nbytes = file->read(file, map_buf, nbytes);
	kunmapaddr(map_buf);
	return nbytes;

fail:
	kunmapaddr(map_buf);
	return -1;
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
	case PROC_STATE_ZOMBIE:
		// you can't kill it if it's already dead
		return 0;

	case PROC_STATE_READY:
		// remove from ready queue
		victim->exit_status = 1;
		pcb_queue_remove(ready_queue, victim);
		pcb_zombify(victim);
		return 0;

	case PROC_STATE_BLOCKED:
		// remove from syscall queue
		victim->exit_status = 1;
		pcb_queue_remove(syscall_queue[victim->syscall], victim);
		pcb_zombify(victim);
		return 0;

	case PROC_STATE_RUNNING:
		// we have met the enemy, and it is us!
		pcb->exit_status = 1;
		pcb_zombify(pcb);
		// we need a new current process
		dispatch();
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
	if (pcb_queue_insert(syscall_queue[SYS_sleep], pcb)) {
		WARN("sleep pcb insert failed");
		return 1;
	}
	pcb->state = PROC_STATE_BLOCKED;

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

static void *pcb_update_heap(intptr_t increment)
{
	char *curr_brk;
	size_t curr_pages, new_pages, new_len;

	new_len = pcb->heap_len + increment;
	new_pages = (new_len + PAGE_SIZE - 1) / PAGE_SIZE;

	curr_brk = pcb->heap_start + pcb->heap_len;
	curr_pages = (pcb->heap_len + PAGE_SIZE - 1) / PAGE_SIZE;
	if (pcb->heap_len == 0)
		curr_pages = 0;

	// do nothing i guess
	if (increment == 0 || new_pages == curr_pages)
		return curr_brk;

	// unmap pages if decreasing
	if (new_pages < curr_pages) {
		void *new_end = pcb->heap_start + new_pages * PAGE_SIZE;
		mem_free_pages(pcb->memctx, new_end);
		pcb->heap_len = new_pages * PAGE_SIZE;
	}

	// map pages if increasing
	else {
		void *curr_end = pcb->heap_start + curr_pages * PAGE_SIZE;
		if (mem_alloc_pages_at(pcb->memctx, new_pages - curr_pages, curr_end,
							   F_PRESENT | F_WRITEABLE | F_UNPRIVILEGED) ==
			NULL)
			return NULL;
		pcb->heap_len = new_pages * PAGE_SIZE;
	}

	return curr_brk;
}

static int sys_brk(void)
{
	RET(void *, brk);
	ARG1(const void *, addr);

	// cannot make heap smaller than start
	if ((const char *)addr < pcb->heap_start) {
		*brk = NULL;
		return 0;
	}

	*brk = pcb_update_heap((intptr_t)addr -
						   ((intptr_t)pcb->heap_start + pcb->heap_len));
	return 0;
}

static int sys_sbrk(void)
{
	RET(void *, brk);
	ARG1(intptr_t, increment);

	*brk = pcb_update_heap(increment);
	return 0;
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

static int sys_seek(void)
{
	RET(long int, ret);
	ARG1(int, fd);
	ARG2(long int, off);
	ARG3(int, whence);

	struct file *file;
	file = get_file_ptr(fd);
	if (file == NULL)
		return -1;

	*ret = file->seek(file, off, whence);
	return 0;
}

static int (*syscall_tbl[N_SYSCALLS])(void) = {
	[SYS_exit] = sys_exit,		 [SYS_waitpid] = sys_waitpid,
	[SYS_fork] = sys_fork,		 [SYS_exec] = sys_exec,
	[SYS_open] = sys_open,		 [SYS_close] = sys_close,
	[SYS_read] = sys_read,		 [SYS_write] = sys_write,
	[SYS_getpid] = sys_getpid,	 [SYS_getppid] = sys_getppid,
	[SYS_gettime] = sys_gettime, [SYS_getprio] = sys_getprio,
	[SYS_setprio] = sys_setprio, [SYS_kill] = sys_kill,
	[SYS_sleep] = sys_sleep,	 [SYS_brk] = sys_brk,
	[SYS_sbrk] = sys_sbrk,		 [SYS_poweroff] = sys_poweroff,
	[SYS_drm] = sys_drm,		 [SYS_ticks] = sys_ticks,
	[SYS_seek] = sys_seek,
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
	pcb->syscall = num;
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
	pcb->syscall = 0;
	current_pcb = pcb;
}
