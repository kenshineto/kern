#include <lib.h>
#include <comus/memory.h>
#include <comus/asm.h>
#include <comus/cpu.h>

#include "idt.h"
#include "pic.h"

#define IDT_SIZE 256

struct idt_entry {
	uint16_t isr_low; // low 16 bits of isr
	uint16_t kernel_cs; // kernel segment selector
	uint8_t ist; // interrupt stack table
	uint8_t flags; // gate type, privilege level, present bit
	uint16_t isr_mid; // middle 16 bits of isr
	uint32_t isr_high; // high 32 bits of isr
	uint32_t reserved;
} __attribute__((packed));

struct idtr {
	uint16_t size;
	uint64_t address;
} __attribute__((packed));

// interrupt gate
#define GATE_64BIT_INT 0x0E
// trap gate
#define GATE_64BIT_TRAP 0x0F

// privilege ring allowed to call interrupt
#define RING0 0x00
#define RING1 0x20
#define RING2 0x40
#define RING3 0x60

// interrupt is present in IDT
#define PRESENT 0x80

__attribute__((aligned(0x10))) static struct idt_entry idt[256];

static struct idtr idtr;
// from idt.S
extern void *isr_stub_table[];

// initialize and load the IDT
void idt_init(void)
{
	// initialize idtr
	idtr.address = (uint64_t)&idt;
	idtr.size = (uint16_t)sizeof(struct idt_entry) * IDT_SIZE - 1;

	// initialize idt
	for (size_t vector = 0; vector < IDT_SIZE; vector++) {
		struct idt_entry *entry = &idt[vector];

		uint64_t isr = (uint64_t)isr_stub_table[vector];
		// interrupts before 0x20 are for cpu exceptions
		uint8_t gate_type = (vector < 0x20) ? GATE_64BIT_TRAP : GATE_64BIT_INT;

		entry->kernel_cs = 0x08; // offset of 1 into GDT
		entry->ist = 0;
		entry->flags = PRESENT | RING0 | gate_type;
		entry->isr_low = isr & 0xffff;
		entry->isr_mid = (isr >> 16) & 0xffff;
		entry->isr_high = (isr >> 32) & 0xffffffff;
		entry->reserved = 0;
	}

	__asm__ volatile("lidt %0" : : "m"(idtr));
}

static void isr_print_regs(regs_t *regs)
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

#define EX_DEBUG 0x01
#define EX_BREAKPOINT 0x03
#define EX_PAGE_FAULT 0x0e

// Intel manual vol 3 ch 6.3.1
char *EXCEPTIONS[] = {
	"Division Error",
	"Debug",
	"NMI",
	"Breakpoint",
	"Overflow",
	"BOUND Range Exceeded",
	"Invalid Opcode",
	"Device Not Available",
	"Double Fault",
	"Coprocessor Segment Overrun",
	"Invalid TSS",
	"Segment Not Present",
	"Stack-Segment Fault",
	"General Protection Fault",
	"Page Fault",
	"Reserved",
	"x87 Floating-Point Error",
	"Alignment Check",
	"Machine Check",
	"SIMD Floaing-Point Exception",
	"Virtualization Exception",
	"Control Protection Exception",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Hypervisor Injection Exception",
	"VMM Communication Exception",
	"Security Exception",
	"Reserved",
};

void idt_exception_handler(uint64_t exception, uint64_t code, regs_t *state)
{
	uint64_t cr2;

	switch (exception) {
	case EX_PAGE_FAULT:
		// page faults store the offending address in cr2
		__asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
		if (!kload_page((void *)cr2))
			return;
	}

	kputs("\n\n!!! EXCEPTION !!!\n");
	kprintf("%#02lX %s\n", exception, EXCEPTIONS[exception]);
	kprintf("Error code %#lX\n", code);

	if (exception == EX_PAGE_FAULT) {
		kprintf("Page fault address: %#016lx\n", cr2);
	}

	kputs("\n");

	isr_print_regs(state);

	kputs("\n");

	log_backtrace_ex((void *)state->rip, (void *)state->rbp);

	while (1) {
		halt();
	}
}

void idt_pic_eoi(uint8_t exception)
{
	pic_eoi(exception - PIC_REMAP_OFFSET);
}

int counter = 0;

void idt_pic_timer(void)
{
	// print a message once we know the timer works
	// but avoid spamming the logs
	if (counter == 3) {
		//kputs("pic timer!\n");
	}
	if (counter <= 3) {
		counter++;
	}
}

void idt_pic_keyboard(void)
{
}

void idt_pic_mouse(void)
{
}
