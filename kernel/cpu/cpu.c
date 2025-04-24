#include <lib.h>
#include <comus/cpu.h>

#include "pic.h"
#include "idt.h"
#include "tss.h"

static inline void fpu_init(void)
{
	size_t cr4;
	uint16_t cw = 0x37F;
	__asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
	cr4 |= 0x200;
	__asm__ volatile("mov %0, %%cr4" ::"r"(cr4));
	__asm__ volatile("fldcw %0" ::"m"(cw));
}

static inline void sse_init(void)
{
	size_t cr0, cr4;
	__asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
	cr0 &= ~0x4; // clear coprocessor emulation
	cr0 |= 0x2; // set coprocessor monitoring
	__asm__ volatile("mov %0, %%cr0" ::"r"(cr0));
	__asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
	cr4 |= 1 << 9; // set CR4.OSFXSR
	cr4 |= 1 << 10; // set CR4.OSXMMEXCPT
	__asm__ volatile("mov %0, %%cr4" ::"r"(cr4));
}

static inline void fxsave_init(void)
{
	static char fxsave_region[512] __attribute__((aligned(16)));
	__asm__ volatile("fxsave %0" ::"m"(fxsave_region));
}

static inline void xsave_init(void)
{
	size_t cr4;
	__asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
	cr4 |= 1 << 18; // set CR4.OSXSAVE
	__asm__ volatile("mov %0, %%cr4" ::"r"(cr4));
}

static inline void avx_init(void)
{
	__asm__ volatile("pushq %rax;"
					 "pushq %rcx;"
					 "pushq %rdx;"
					 "xorq %rcx, %rcx;"
					 "xgetbv;"
					 "orq $7, %rax;"
					 "xsetbv;"
					 "popq %rdx;"
					 "popq %rcx;"
					 "popq %rax;");
}

void cpu_init(void)
{
	struct cpu_feat feats;
	cpu_feats(&feats);

	pic_remap();
	idt_init();
	if (feats.fpu)
		fpu_init();
	if (feats.sse) {
		sse_init();
		fxsave_init();
	}
	if (feats.xsave) {
		xsave_init();
		if (feats.avx)
			avx_init();
	}

	tss_init();
}

void cpu_report(void)
{
	char vendor[12], name[48];
	struct cpu_feat feats;

	cpu_name(name);
	cpu_vendor(vendor);
	cpu_feats(&feats);

	kprintf("CPU\n");
	kprintf("Name: %.*s\n", 48, name);
	kprintf("Vendor: %.*s\n", 12, vendor);
	kputs("Features:");
	if (feats.fpu)
		kputs(" FPU");
	if (feats.mmx)
		kputs(" MMX");
	if (feats.sse)
		kputs(" SSE");
	if (feats.sse2)
		kputs(" SSE2");
	if (feats.sse3)
		kputs(" SSE3");
	if (feats.ssse3)
		kputs(" SSSE3");
	if (feats.sse41)
		kputs(" SSE4.1");
	if (feats.sse42)
		kputs(" SSE4.2");
	if (feats.sse4a)
		kputs(" SSE4a");
	if (feats.avx)
		kputs(" AVX");
	if (feats.xsave)
		kputs(" XSAVE");
	if (feats.avx2)
		kputs(" AVX2");
	if (feats.avx512)
		kputs(" AVX-512");
	kputs("\n\n");
}

static inline void cpuid(uint32_t leaf, uint32_t *eax, uint32_t *ebx,
						 uint32_t *ecx, uint32_t *edx)
{
	__asm__ volatile("cpuid"
					 : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
					 : "0"(leaf));
}

static inline void cpuid_count(uint32_t leaf, uint32_t subleaf, uint32_t *eax,
							   uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
	__asm__ volatile("cpuid"
					 : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
					 : "0"(leaf), "2"(subleaf));
}

void cpu_vendor(char vendor[12])
{
	uint32_t ignore;
	cpuid(0, &ignore, (uint32_t *)(vendor + 0), (uint32_t *)(vendor + 8),
		  (uint32_t *)(vendor + 4));
}

void cpu_name(char name[48])
{
	cpuid(0x80000002, (uint32_t *)(name + 0), (uint32_t *)(name + 4),
		  (uint32_t *)(name + 8), (uint32_t *)(name + 12));
	cpuid(0x80000003, (uint32_t *)(name + 16), (uint32_t *)(name + 20),
		  (uint32_t *)(name + 24), (uint32_t *)(name + 28));
	cpuid(0x80000004, (uint32_t *)(name + 32), (uint32_t *)(name + 36),
		  (uint32_t *)(name + 40), (uint32_t *)(name + 44));
}

void cpu_feats(struct cpu_feat *feats)
{
	memset(feats, 0, sizeof(struct cpu_feat));

	uint32_t ignore;
	uint32_t ecx_1, edx_1;
	uint32_t ebx_7;

	cpuid(1, &ignore, &ignore, &ecx_1, &edx_1);
	cpuid_count(7, 0, &ignore, &ebx_7, &ignore, &ignore);

	feats->fpu = edx_1 & (1 << 0) ? 1 : 0;
	feats->mmx = edx_1 & (1 << 23) ? 1 : 0;
	feats->sse = edx_1 & (1 << 25) ? 1 : 0;
	feats->sse2 = edx_1 & (1 << 26) ? 1 : 0;
	feats->sse3 = ecx_1 & (1 << 0) ? 1 : 0;
	feats->ssse3 = ecx_1 & (1 << 9) ? 1 : 0;
	feats->sse41 = ecx_1 & (1 << 19) ? 1 : 0;
	feats->sse42 = ecx_1 & (1 << 20) ? 1 : 0;
	feats->sse4a = ecx_1 & (1 << 6) ? 1 : 0;
	feats->avx = ecx_1 & (1 << 28) ? 1 : 0;
	feats->xsave = ecx_1 & (1 << 26) ? 1 : 0;
	feats->avx2 = ebx_7 & (1 << 5) ? 1 : 0;
	feats->avx512 = ebx_7 & (7 << 16) ? 1 : 0;
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
	kprintf("rflags: %#016lx (%lu)\n", regs->rflags, regs->rflags);
	kputs("rflags: ");
	if (regs->rflags & (1 << 0))
		kputs("CF ");
	if (regs->rflags & (1 << 2))
		kputs("PF ");
	if (regs->rflags & (1 << 4))
		kputs("AF ");
	if (regs->rflags & (1 << 6))
		kputs("ZF ");
	if (regs->rflags & (1 << 7))
		kputs("SF ");
	if (regs->rflags & (1 << 8))
		kputs("TF ");
	if (regs->rflags & (1 << 9))
		kputs("IF ");
	if (regs->rflags & (1 << 10))
		kputs("DF ");
	if (regs->rflags & (1 << 11))
		kputs("OF ");
	if (regs->rflags & (3 << 12))
		kputs("IOPL ");
	if (regs->rflags & (1 << 14))
		kputs("NT ");
	if (regs->rflags & (1 << 15))
		kputs("MD ");
	if (regs->rflags & (1 << 16))
		kputs("RF ");
	if (regs->rflags & (1 << 17))
		kputs("VM ");
	if (regs->rflags & (1 << 18))
		kputs("AC ");
	if (regs->rflags & (1 << 19))
		kputs("VIF ");
	if (regs->rflags & (1 << 20))
		kputs("VIP ");
	if (regs->rflags & (1 << 21))
		kputs("ID ");
	kputs("\n");
}
