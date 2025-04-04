#include <comus/cpu.h>
#include <comus/memory.h>
#include <lib.h>
#include <stdio.h>

void main(void)
{
	cpu_init();
	memory_init(NULL);
	printf("halting...\n");
	while(1);
}
