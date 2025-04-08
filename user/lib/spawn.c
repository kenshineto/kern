#include <stdio.h>
#include <error.h>
#include <unistd.h>

int wait(int32_t *status)
{
	return (waitpid(0, status));
}

int spawn(uint_t prog, char **args)
{
	int32_t pid;

	pid = fork();
	if (pid != 0) {
		// failure, or we are the parent
		return (pid);
	}

	// we are the child
	pid = getpid();

	// child inherits parent's priority level

	exec(prog, args);

	// uh-oh....

	fprintf(stderr, "Child %d exec() #%u failed\n", pid, prog);

	exit(EXIT_FAILURE);
}
