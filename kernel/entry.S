	.globl _start
	.globl kernel_pml4
	.globl kernel_pdpt_0
	.globl kernel_pd_0
	.globl kernel_pt_0
	.globl paging_pt
	.globl bootstrap_pt
	.extern main
	.extern GDT

	.section .bss

	# kernel page tables
	.align 4096
kernel_pml4:            # reserve memory for initial 512 pml4 entires
	.skip 4096
kernel_pdpt_0:          # reserve memory for initial 512 pdpt entires
	.skip 4096
kernel_pd_0:            # reserve memory for initial 512 pd entries
	.skip 4096
kernel_pt_0:            # reserve memory for initial 512 pt entries
	.skip 4096
paging_pt:              # reserve pages for pager mappings
	.skip 4096
bootstrap_pt:           # reserve pages to bootstrap pager
	.skip 4096

	# kernel stack
	.align 16
kern_stack_start:
	.skip 8192
kern_stack_end:

	.section .rodata
	.align 16

	# access bits
	.set PRESENT,   1 << 7
	.set NOT_SYS,   1 << 4
	.set EXEC,      1 << 3
	.set DC,        1 << 2
	.set RW,        1 << 1
	.set ACCESSED,  1 << 0

	# flag bits
	.set GRAN_4K,   1 << 7
	.set SZ_32,     1 << 6
	.set LONG_MODE, 1 << 5

	# kernel gdt (long mode)
GDT:
	GDT.Null:
		.quad 0

	GDT.Code:
		.byte 0
		.byte PRESENT | NOT_SYS | EXEC | RW
		.byte GRAN_4K | LONG_MODE | 0xF
		.byte 0

	GDT.Data:
		.long 0xFFFF
		.byte 0
		.byte PRESENT | NOT_SYS | RW
		.byte GRAN_4K | SZ_32 | 0xF
		.byte 0

	GDT.TSS:
		.long 0x00000068
		.long 0x00CF8900

	GDT.Pointer:
		.short . - GDT - 1
		.quad GDT - .

	.section .text
	.code32

_start:
    # enable interrupts
	cli

    # setup stack
	movl	$kern_stack_end, %esp
	movl	$kern_stack_end, %ebp

	# save multiboot (if using multiboot)
	pushl	$0
	push 	%ebx
	pushl	$0
	push 	%eax

	# zero out kernel page table
	movl	$0x1000, %edi    # kernel is loaded at 0x1000
	movl	%edi, %cr3
	xorl	%eax, %eax
	movl	$4096, %ecx      # zero 4096 pages
	rep		stosl
	movl	%cr3, %edi

	# identity map kernel
	movl	$kernel_pdpt_0 + 3, (%edi)      # Set the uint32_t at the desination index to 0x2003.
	movl	$kernel_pdpt_0, %edi            # Add 0x1000 to the desination index.
	movl	$kernel_pd_0 + 3, (%edi)        # Set the uint32_t at the desination index to 0x3003.
	movl	$kernel_pd_0, %edi              # Add 0x1000 to the desination index.
	movl	$kernel_pt_0 + 3, (%edi)        # Set the uint32_t at the desination index to 0x4003.
	movl	$kernel_pt_0, %edi              # Add 0x1000 to the desination index.

	movl	$0x00000003, %ebx       # Entry value to set
	movl	$512, %ecx              # Loop count

set_entry:
	# set entires in mapping
	movl	%ebx, (%edi)            # Set the uint32_t at the desination index to the B-register
	addl	$0x1000, %ebx           # Add 0x1000 to the B-register
	addl	$8, %edi                # Add eight to the desination index
	loop	set_entry

	# enable page address extension
	movl	%cr4, %eax
	orl		$(1 << 5), %eax
	movl	%eax, %cr4

	# enable long mode
	movl	$0xC0000080, %eax
	rdmsr
	orl		$(1 << 8), %eax
	wrmsr

	# enable paging
	movl	%cr0, %eax
	orl		$(1 << 31), %eax
	movl	%eax, %cr0

	# load gdt
	lgdt	GDT.Pointer
	ljmp	$16, $code64

	.code64
code64:

	movw	$16, %dx        # set segment registers
	movw	%dx, %ds
	movw	%dx, %ss

	xorq	%rbp, %rbp      # set ebp to 0 so we know where to end stack traces

	pop		%rdi            # pop possible multiboot header
	pop		%rsi

	sti
	call	main
	cli

 halt:
	hlt
	jmp		halt

