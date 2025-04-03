#include <comus/memory.h>
#include <comus/asm.h>

#include "paging.h"
#include "virtalloc.h"
#include "physalloc.h"

void memory_init(struct memory_map *map)
{
	cli();
	paging_init();
	virtaddr_init();
	physalloc_init(map);
	sti();
}
