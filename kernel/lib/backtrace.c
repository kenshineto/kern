#include <lib.h>
#include <comus/mboot.h>

struct stackframe {
	struct stackframe *rbp;
	void *rip;
};

extern char kern_stack_start[];
extern char kern_stack_end[];

#define VALID(frame) (frame && (char*)(frame) >= kern_stack_start && ((char*)(frame) <= kern_stack_end))

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
	if (!VALID(frame))
		return 0;
	if (len > 0) {
		dst[0] = ip;
	}
	size_t i;
	for (i = 1; VALID(frame) && i < len; i++) {
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
	if (!VALID(frame))
		return;
	kputs("Stack trace:\n");
	kprintf("  %p\t%s\n", ip, mboot_get_elf_sym((uint64_t)ip));
	while (VALID(frame)) {
		kprintf("  %p\t%s\n", frame->rip,
				mboot_get_elf_sym((uint64_t)frame->rip));
		frame = frame->rbp;
	}
}
