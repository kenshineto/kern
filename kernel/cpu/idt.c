#include <lib.h>
#include <comus/memory.h>
#include <comus/asm.h>
#include <comus/cpu.h>
#include <comus/drivers/ps2.h>
#include <comus/drivers/pit.h>
#include <comus/procs.h>
#include <comus/memory.h>

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
extern void *isr_stub_table[];
extern char kern_stack_start[];
extern char kern_stack_end[];

// current register state on interrupt
static struct cpu_regs *state;

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

		entry->kernel_cs = 0x08; // offset of 1 into GDT
		entry->ist = 0;
		entry->flags = PRESENT | RING0 | GATE_64BIT_INT;
		entry->isr_low = isr & 0xffff;
		entry->isr_mid = (isr >> 16) & 0xffff;
		entry->isr_high = (isr >> 32) & 0xffffffff;
		entry->reserved = 0;

		if (vector == 0x80)
			entry->flags |= RING3;
	}

	__asm__ volatile("lidt %0" : : "m"(idtr));
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

__attribute__((noreturn)) void idt_exception_handler(uint64_t exception,
													 uint64_t code)
{
	uint64_t cr2;

	switch (exception) {
	case EX_PAGE_FAULT:
		// page faults store the offending address in cr2
		__asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
		break;
	}

	kputs("\n\n!!! EXCEPTION !!!\n");
	kprintf("%#02lX %s\n", exception, EXCEPTIONS[exception]);
	kprintf("Error code %#lX\n", code);

	if (exception == EX_PAGE_FAULT) {
		kprintf("Page fault address: %#016lx\n", cr2);
	}

	kputs("\n");

	cpu_print_regs(state);

	kputs("\n");

	log_backtrace_ex((void *)state->rip, (void *)state->rbp);

	while (1) {
		halt();
	}
}

void isr_save(struct cpu_regs *regs)
{
	// make sure were in the kernel memory context
	mem_ctx_switch(kernel_mem_ctx);

	// save pointer to registers
	state = regs;

	// if we have a kernel stack pointer
	// we should return to not save kernel context
	// data to userspace
	if (regs->rsp >= (size_t)kern_stack_start &&
		regs->rsp <= (size_t)kern_stack_end)
		return;

	// save registers in current_pcb
	if (current_pcb != NULL)
		current_pcb->regs = *regs;
}

void idt_pic_eoi(uint8_t exception)
{
	pic_eoi(exception - PIC_REMAP_OFFSET);
}

void idt_pic_timer(void)
{
	ticks++;
	pcb_on_tick();
}

void idt_pic_keyboard(void)
{
	ps2kb_recv();
}

void idt_pic_mouse(void)
{
	ps2mouse_recv();
}
