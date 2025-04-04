/**
 * @file idt.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * IDT functions
 */

#ifndef IDT_H_
#define IDT_H_

#include <stdint.h>

struct isr_regs {
	uint64_t r15;
	uint64_t r14;
	uint64_t r13;
	uint64_t r12;
	uint64_t r11;
	uint64_t r10;
	uint64_t r9;
	uint64_t r8;
	uint64_t rbp;
	uint64_t rdi;
	uint64_t rsi;
	uint64_t rdx;
	uint64_t rcx;
	uint64_t rbx;
	uint64_t rax;

	uint64_t rip;
	uint64_t cs;
	uint64_t rflags;
	uint64_t rsp;
	uint64_t ss;
};

struct rflags {
	uint64_t cf : 1, : 1, pf : 1, : 1, af : 1, : 1, zf : 1, sf : 1,

		tf : 1, if_ : 1, df : 1, of : 1, iopl : 2, nt : 1, md : 1,

		rf : 1, vm : 1, ac : 1, vif : 1, vip : 1, id : 1, : 42;
};

void idt_init(void);

#endif /* idt.h */
