#include <unistd.h>
#include <stdio.h>

int main(void)
{
	printf("im going to print some a's and b's. get ready\n");

	int pid = fork();
	if (pid < 0) {
		fprintf(stderr, "fork failed!\n");
		return 1;
	}

	for (int i = 0; i < 10; i++) {
		putchar(pid == 0 ? 'a' : 'b');
		putchar('\n');
	}

	return 0;
}
