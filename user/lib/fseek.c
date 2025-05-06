#include <stdio.h>
#include <unistd.h>

int fseek(FILE *stream, long off, int whence)
{
	int fd;
	fd = (uintptr_t)stream;
	return seek(fd, off, whence);
}

long ftell(FILE *stream)
{
	int fd;
	fd = (uintptr_t)stream;
	return seek(fd, 0, SEEK_CUR);
}
