#include <stdio.h>
#include <unistd.h>

#define MAX_ARGS 4

static long running = 0;

struct proc {
	pid_t pid;
	const char *filename;
	const char *args[MAX_ARGS];
};

static struct proc spawn_table[] = {
	// apple
	{ 0, "bin/apple", { NULL } },
	// end,
	{ 0, NULL, { NULL } },
};

static int spawn(const char *filename, const char **args)
{
	int ret;

	// fork init
	if ((ret = fork()) != 0)
		return ret;

	// call exec
	if ((ret = exec(filename, args)))
		exit(ret);

	// should not happen!
	exit(1);
}

static void spawn_proc(struct proc *proc)
{
	int ret;

	// update running on respawn
	if (proc->pid)
		running--;

	// attempt to fork / exec
	ret = spawn(proc->filename, proc->args);

	// handle result
	if (ret < 0) {
		printf("init: cannot exec '%s': %d\n", proc->filename, ret);
		proc->pid = 0;
	} else {
		running++;
		proc->pid = ret;
	}
}

static void spawn_proc_loop(struct proc *proc)
{
	while (proc->pid == 0)
		spawn_proc(proc);
}

static void spawn_all(void)
{
	struct proc *proc;
	for (proc = spawn_table; proc->filename != NULL; proc++) {
		spawn_proc_loop(proc);
	}
}

static struct proc *get_proc(pid_t pid)
{
	struct proc *proc;
	for (proc = spawn_table; proc->filename != NULL; proc++) {
		if (proc->pid == pid)
			return proc;
	}
	return NULL;
}

int main(void)
{
	// spawn our processes
	spawn_all();

	// clean up dead on restart ours
	while (1) {
		struct proc *proc;
		int pid, status;

		// dont wait if nothing running
		if (running) {
			pid = waitpid(0, &status);
			// ???
			if (pid < 0)
				continue;
		} else {
			spawn_all();
			continue;
		}

		printf("init: pid %d exited with %d\n", pid, status);

		// figure out if this is one of ours
		proc = get_proc(pid);
		if (proc == NULL)
			continue;

		spawn_proc_loop(proc);
	}

	// failed to launch anythingvery very bad!
	return 1;
}
