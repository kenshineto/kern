#include <stdio.h>
#include <unistd.h>

FILE *fopen(const char *restrict filename, const char *restrict modes)
{
	int flags = 0;
	int fd;

	char c;
	while (c = *modes, c) {
		switch (c) {
		case 'r':
			flags |= O_RDONLY;
			break;
		case 'w':
			flags |= O_CREATE | O_WRONLY;
			break;
		case 'a':
			flags |= O_APPEND;
			break;
		case '+':
			flags |= O_RDWR;
			break;
		default:
			return NULL;
		}
	}

	if (flags == 0)
		return NULL;

	fd = open(filename, flags);
	if (fd < 0)
		return NULL;

	return (FILE *)(uintptr_t)fd;
}
