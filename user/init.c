#include <stdio.h>
#include <unistd.h>

#define MAX_ARGS 4

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

int main(void)
{
	struct proc *proc;

	// spawn our processes
	for (proc = spawn_table; proc->filename != NULL; proc++) {
		int pid;
		pid = spawn(proc->filename, proc->args);
		if (pid < 0)
			fprintf(stderr, "init: cannot exec '%s': %d\n", proc->filename,
					pid);
		proc->pid = pid;
	}

	// clean up dead on restart ours
	while (1) {
		int pid, status;

		pid = waitpid(0, &status);
		if (pid < 0)
			continue;

		printf("init: pid %d exited with %d\n", pid, status);

		// figure out if this is one of ours
		for (proc = spawn_table; proc->filename != NULL; proc++) {
			if (proc->pid == pid) {
				proc->pid = 0;
				pid = spawn(proc->filename, proc->args);
				if (pid < 0)
					fprintf(stderr, "init: cannot exec '%s': %d\n",
							proc->filename, pid);
				proc->pid = pid;
				break;
			}
		}
	}

	// very very bad!
	return 1;
}
