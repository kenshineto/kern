#include <comus/memory.h>
#include <comus/asm.h>
#include <comus/mboot.h>
#include <lib.h>

#include "paging.h"
#include "virtalloc.h"
#include "physalloc.h"

void memory_init(void)
{
	struct memory_map mmap;
	if (mboot_get_mmap(&mmap))
		panic("failed to load memory map");

	cli();
	paging_init();
	virtaddr_init();
	physalloc_init(&mmap);
	sti();
}
