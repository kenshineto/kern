/**
 * @file cpu.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * CPU initalization functions
 */

#ifndef _CPU_H
#define _CPU_H

#include <stdint.h>

union cpu_rflags {
	uint64_t raw;
	struct {
		uint64_t cf : 1;
		uint64_t : 1;
		uint64_t pf : 1;
		uint64_t : 1;
		uint64_t af : 1;
		uint64_t : 1;
		uint64_t zf : 1;
		uint64_t sf : 1;
		uint64_t tf : 1;
		uint64_t if_ : 1;
		uint64_t df : 1;
		uint64_t of : 1;
		uint64_t iopl : 2;
		uint64_t nt : 1;
		uint64_t md : 1;
		uint64_t rf : 1;
		uint64_t vm : 1;
		uint64_t ac : 1;
		uint64_t vif : 1;
		uint64_t vip : 1;
		uint64_t id : 1;
		uint64_t : 42;
	};
};

struct cpu_regs {
	// registers
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
	// instruction pointer
	uint64_t rip;
	// code segment
	uint64_t cs;
	// rflags
	union cpu_rflags rflags;
	// stack pointer
	uint64_t rsp;
	// stack segment
	uint64_t ss;
};

/**
 * Initalize current cpu
 */
void cpu_init(void);

/**
 * Dump registers to output
 */
void cpu_print_regs(struct cpu_regs *regs);

#endif /* cpu.h */
