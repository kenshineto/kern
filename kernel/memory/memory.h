
#include <comus/memory.h>
#include "virtalloc.h"

struct mem_ctx_s {
	// page tables
	volatile char *pml4;
	// virt addr allocator
	struct virt_ctx virtctx;
	// linked list
	struct mem_ctx_s *next;
	struct mem_ctx_s *prev;
};
