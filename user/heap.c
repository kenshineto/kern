
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define PAGE_SIZE 4096

int main(void)
{
	void *start, *brk;

	start = sbrk(0);
	printf("heap start: %p\n", start);

	// test extending
	if (sbrk(PAGE_SIZE) == NULL) {
		fprintf(stderr, "failed to extend break\n");
		return 1;
	}

	brk = sbrk(0);
	printf("new break: %p\n", brk);

	// test reextending
	if (sbrk(PAGE_SIZE) == NULL) {
		fprintf(stderr, "failed to extend break\n");
		return 1;
	}

	brk = sbrk(0);
	printf("new new break: %p\n", brk);

	// test shrinking
	if (sbrk(-PAGE_SIZE) == NULL) {
		fprintf(stderr, "failed to shrink break\n");
		return 1;
	}

	brk = sbrk(0);
	printf("new new new break: %p\n", brk);

	// test write
	memset(start, 1, PAGE_SIZE);

	return 0;
}
