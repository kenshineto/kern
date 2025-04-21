#include <stdio.h>
#include <unistd.h>

FILE *stdin = (void *)0;

int getchar(void)
{
	return fgetc(stdin);
}

int getc(FILE *stream)
{
	return fgetc(stream);
}

int fgetc(FILE *stream)
{
	int c;
	if (fread(&c, 1, 1, stream) < 1)
		return EOF;
	return c;
}

char *gets(char *str)
{
	char *s = str;
	while (1) {
		char c = fgetc(stdin);
		if (c == '\n' || c == EOF || c == '\0')
			break;
		*(str++) = c;
	}
	*str = '\0';
	return s;
}

char *fgets(char *restrict str, int size, FILE *stream)
{
	if (size < 1)
		return NULL;

	char *s = str;
	while (size > 1) {
		char c = fgetc(stream);
		if (c == '\n' || c == EOF || c == '\0')
			break;
		*(str++) = c;
		size--;
	}

	*str = '\0';
	return s;
}

size_t fread(void *restrict ptr, size_t size, size_t n, FILE *restrict stream)
{
	int fd = (uintptr_t)stream;
	char *restrict buf = ptr;

	for (size_t i = 0; i < n; i++)
		if (read(fd, buf + i * size, size) < 1)
			return i;

	return n;
}
