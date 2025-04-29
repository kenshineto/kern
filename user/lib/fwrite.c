#include <stdio.h>
#include <unistd.h>

FILE *stdout = (void *)1;
FILE *stderr = (void *)2;

int putchar(int c)
{
	return putc(c, stdout);
}

int putc(int c, FILE *stream)
{
	return fputc(c, stream);
}

int fputc(int c, FILE *stream)
{
	if (fwrite(&c, 1, 1, stream) < 1)
		return EOF;
	return c;
}

int puts(const char *str)
{
	int res;
	res = fputs(str, stdout);
	if (res == EOF)
		return res;
	res = fputc('\n', stdout);
	if (res == EOF)
		return res;
	return 0;
}

int fputs(const char *str, FILE *stream)
{
	int res;
	while (*str) {
		res = fputc(*str++, stream);
		if (res == EOF)
			return res;
	}
	return 0;
}

size_t fwrite(const void *restrict ptr, size_t size, size_t n,
			  FILE *restrict stream)
{
	int fd = (uintptr_t)stream;
	const char *restrict buf = ptr;

	for (size_t i = 0; i < n; i++)
		if (write(fd, buf + i * size, size) < 1)
			return i;

	return n;
}
