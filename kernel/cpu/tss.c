#include "lib/kstring.h"
#include <stdint.h>

#include "tss.h"

struct sys_seg_descriptor {
	uint64_t limit0_15 : 16;
	uint64_t base0_15 : 16;
	uint64_t base16_23 : 8;
	uint64_t type : 4;
	uint64_t : 1;
	uint64_t DPL : 2;
	uint64_t present : 1;
	uint64_t limit16_19: 4;
	uint64_t available : 1;
	uint64_t : 1;
	uint64_t : 1;
	uint64_t gran : 1;
	uint64_t base24_31 : 8;
	uint64_t base32_63 : 32;
	uint64_t : 32;
} __attribute__((packed));

struct tss {
	uint64_t : 32;
	uint64_t rsp0 : 64;
	uint64_t rsp1 : 64;
	uint64_t rsp2 : 64;
	uint64_t : 64;
	uint64_t ist1 : 64;
	uint64_t ist2 : 64;
	uint64_t ist3 : 64;
	uint64_t ist4 : 64;
	uint64_t ist5 : 64;
	uint64_t ist6 : 64;
	uint64_t ist7 : 64;
	uint64_t : 64;
	uint64_t : 16;
	uint64_t iopb : 16;
} __attribute__((packed));

// tss entry
static volatile struct tss tss;

// gdt entries
extern volatile uint8_t GDT[];
static volatile struct sys_seg_descriptor *GDT_TSS;

// kernel stack pointer
extern char kern_stack_end[];

void tss_init(void) {
	uint64_t base = (uint64_t) &tss;
	uint64_t limit = sizeof tss - 1;

	// setup tss entry
	memsetv(&tss, 0, sizeof(struct tss));
	tss.rsp0 = (uint64_t) kern_stack_end;

	// map tss into gdt
	GDT_TSS = (volatile struct sys_seg_descriptor *) (GDT + 0x28);
	memsetv(GDT_TSS, 0, sizeof(struct sys_seg_descriptor));
	GDT_TSS->limit0_15 = limit & 0xFFFF;
	GDT_TSS->base0_15 = base & 0xFFFF;
	GDT_TSS->base16_23 = (base >> 16) & 0xFF;
	GDT_TSS->type = 0x9;
	GDT_TSS->DPL = 0;
	GDT_TSS->present = 1;
	GDT_TSS->limit16_19 = (limit >> 16) & 0xF;
	GDT_TSS->available = 0;
	GDT_TSS->gran = 0;
	GDT_TSS->base24_31 = (base >> 24) & 0xFF;
	GDT_TSS->base32_63 = (base >> 32) & 0xFFFFFFFF;

	tss_flush();
}

void tss_set_stack(uint64_t stack) {
	tss.rsp0 = stack;
}
