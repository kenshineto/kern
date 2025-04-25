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
#include <stdbool.h>

struct cpu_feat {
	// floating point
	uint32_t fpu : 1;
	// simd
	uint32_t mmx : 1;
	uint32_t sse : 1;
	uint32_t sse2 : 1;
	uint32_t sse3 : 1;
	uint32_t ssse3 : 1;
	uint32_t sse41 : 1;
	uint32_t sse42 : 1;
	uint32_t sse4a : 1;
	uint32_t avx : 1;
	uint32_t xsave : 1;
	uint32_t avx2 : 1;
	uint32_t avx512 : 1;
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
	uint64_t rflags;
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
 * Report all cpu information
 */
void cpu_report(void);

/**
 * @returns the name/model of the cpu
 */
void cpu_name(char name[48]);

/**
 * @returns the vendor of the cpu
 */
void cpu_vendor(char vendor[12]);

/**
 * @returns get features of the cpu
 */
void cpu_feats(struct cpu_feat *feats);

/**
 * Dump registers to output
 */
void cpu_print_regs(struct cpu_regs *regs);

/**
 * Return from a syscall handler back into userspace
 */
__attribute__((noreturn))
void syscall_return(void);

#endif /* cpu.h */
