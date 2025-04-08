#include <lib.h>

struct stackframe {
	struct stackframe *rbp;
	void *rip;
};

size_t backtrace(void **dst, size_t len)
{
	struct stackframe *rbp;
	__asm__("mov %%rbp, %0" : "=r"(rbp));
	return backtrace_ex(dst, len, rbp->rip, rbp->rbp);
}

size_t backtrace_ex(void **dst, size_t len, void *ip, void *bp)
{
	struct stackframe *frame = bp;
	__asm__("mov %%rbp, %0" : "=r"(frame));
	if (len > 0) {
		dst[0] = ip;
	}
	size_t i;
	for (i = 1; frame && i < len; i++) {
		dst[i] = frame->rip;
		frame = frame->rbp;
	}
	return i;
}

void log_backtrace(void)
{
	struct stackframe *rbp;
	__asm__("mov %%rbp, %0" : "=r"(rbp));
	log_backtrace_ex(rbp->rip, rbp->rbp);
}

void log_backtrace_ex(void *ip, void *bp)
{
	struct stackframe *frame = bp;
	kputs("Stack trace:\n");
	kprintf("  %p\n", ip);
	while (frame) {
		kprintf("  %p\n", frame->rip);
		frame = frame->rbp;
	}
}
