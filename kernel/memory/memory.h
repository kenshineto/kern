
#include <comus/memory.h>
#include "virtalloc.h"

struct mem_ctx_s {
	struct virt_ctx *virtctx;
	volatile char *pml4;
};
