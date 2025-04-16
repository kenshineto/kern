#include <lib.h>
#include <comus/cpu.h>

#include "fpu.h"
#include "pic.h"
#include "idt.h"

void cpu_init(void)
{
	pic_remap();
	idt_init();
	fpu_init();
}

void cpu_print_regs(struct cpu_regs *regs)
{
	kprintf("rax: %#016lx (%lu)\n", regs->rax, regs->rax);
	kprintf("rbx: %#016lx (%lu)\n", regs->rbx, regs->rbx);
	kprintf("rcx: %#016lx (%lu)\n", regs->rcx, regs->rcx);
	kprintf("rdx: %#016lx (%lu)\n", regs->rdx, regs->rdx);
	kprintf("rsi: %#016lx (%lu)\n", regs->rsi, regs->rsi);
	kprintf("rdi: %#016lx (%lu)\n", regs->rdi, regs->rdi);
	kprintf("rsp: %#016lx (%lu)\n", regs->rsp, regs->rsp);
	kprintf("rbp: %#016lx (%lu)\n", regs->rbp, regs->rbp);
	kprintf("r8 : %#016lx (%lu)\n", regs->r8, regs->r8);
	kprintf("r9 : %#016lx (%lu)\n", regs->r9, regs->r9);
	kprintf("r10: %#016lx (%lu)\n", regs->r10, regs->r10);
	kprintf("r11: %#016lx (%lu)\n", regs->r11, regs->r11);
	kprintf("r12: %#016lx (%lu)\n", regs->r12, regs->r12);
	kprintf("r13: %#016lx (%lu)\n", regs->r13, regs->r13);
	kprintf("r14: %#016lx (%lu)\n", regs->r14, regs->r14);
	kprintf("r15: %#016lx (%lu)\n", regs->r15, regs->r15);
	kprintf("rip: %#016lx (%lu)\n", regs->rip, regs->rip);
	kprintf("rflags: %#016lx (%lu)\n", (uint64_t)regs->rflags.raw,
			(uint64_t)regs->rflags.raw);
	kputs("rflags: ");
	if (regs->rflags.cf)
		kputs("CF ");
	if (regs->rflags.pf)
		kputs("PF ");
	if (regs->rflags.af)
		kputs("AF ");
	if (regs->rflags.zf)
		kputs("ZF ");
	if (regs->rflags.sf)
		kputs("SF ");
	if (regs->rflags.tf)
		kputs("TF ");
	if (regs->rflags.if_)
		kputs("IF ");
	if (regs->rflags.df)
		kputs("DF ");
	if (regs->rflags.of)
		kputs("OF ");
	if (regs->rflags.iopl)
		kputs("IOPL ");
	if (regs->rflags.nt)
		kputs("NT ");
	if (regs->rflags.md)
		kputs("MD ");
	if (regs->rflags.rf)
		kputs("RF ");
	if (regs->rflags.vm)
		kputs("VM ");
	if (regs->rflags.ac)
		kputs("AC ");
	if (regs->rflags.vif)
		kputs("VIF ");
	if (regs->rflags.vip)
		kputs("VIP ");
	if (regs->rflags.id)
		kputs("ID ");
	kputs("\n");
}
