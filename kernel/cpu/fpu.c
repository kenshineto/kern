#include <lib.h>

#include "fpu.h"

void fpu_init(void)
{
	size_t cr4;
	uint16_t cw = 0x37F;
	__asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
	cr4 |= 0x200;
	__asm__ volatile("mov %0, %%cr4" ::"r"(cr4));
	__asm__ volatile("fldcw %0" ::"m"(cw));
}
