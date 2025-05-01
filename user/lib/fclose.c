#include <stdio.h>
#include <unistd.h>

void fclose(FILE *stream)
{
	int fd;

	fd = (uintptr_t)stream;
	close(fd);
}
