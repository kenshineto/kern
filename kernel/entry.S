	.globl _start
	.globl kernel_pml4
	.globl kernel_pdpt_0
	.globl kernel_pd_0
	.globl kernel_pt_0
	.globl paging_pt
	.globl bootstrap_pt
	.extern main
	.extern GDT

	.section .multiboot
	.align 8

	# multiboot header
mb_start:
	# magic
	.long 0xe85250d6
	.long 0
	.long mb_end - mb_start
	.long 0x100000000 - (0xe85250d6 + (mb_end - mb_start))
	# null
	.short 0
	.short 0
	.long 8
mb_end:

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

	.section .data
	.align 16

	# access bits
	.set PRESENT,   1 << 7
	.set NOT_SYS,   1 << 4
	.set EXEC,      1 << 3
	.set DC,        1 << 2
	.set RW,        1 << 1
	.set ACCESSED,  1 << 0

	# privlage level (access bit)
	.set RING0,     0 << 5
	.set RING1,     1 << 5
	.set RING2,     2 << 5
	.set RING3,     3 << 5

	# flag bits
	.set GRAN_4K,   1 << 7
	.set SZ_32,     1 << 6
	.set LONG_MODE, 1 << 5

	# kernel gdt (long mode)
GDT:
	# Null Segment (0x00)
	.equ GDT.Null, . - GDT
	.quad 0

	# Kernel Code segment (0x08)
	.equ GDT.Code, . - GDT
	.long 0xFFFF
	.byte 0
	.byte PRESENT | NOT_SYS | EXEC | RW | RING0
	.byte GRAN_4K | LONG_MODE | 0xF
	.byte 0

	# Kernel Data segment (0x10)
	.equ GDT.Data, . - GDT
	.long 0xFFFF
	.byte 0
	.byte PRESENT | NOT_SYS | RW | RING0
	.byte GRAN_4K | SZ_32 | 0xF
	.byte 0

	# User Code Segment (0x18)
	.equ GDT.UserCode, . - GDT
	.long 0xFFFF
	.byte 0
	.byte PRESENT | NOT_SYS | EXEC | RW | RING3
	.byte GRAN_4K | LONG_MODE | 0xF
	.byte 0

	# User Data Segment (0x20)
	.equ GDT.UserData, . - GDT
	.long 0xFFFF
	.byte 0
	.byte PRESENT | NOT_SYS | RW | RING3
	.byte GRAN_4K | SZ_32 | 0xF
	.byte 0

	# TSS segment (0x28)
	.equ GDT.TSS, . - GDT
	.quad 0 # to be modified in kernel

	# GDT Pointer
	.equ GDT.Pointer, .
	.word . - GDT - 1
	.quad GDT

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
	movl	$0x1000, %edi    # pml4 located at 0x1000
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

	movl	$0x03, %ebx         # Entry value to set
	movl	$512, %ecx          # Loop count

_start.SetEntry:
	# set entires in mapping
	movl	%ebx, (%edi)            # Set the uint32_t at the desination index to the B-register
	addl	$0x1000, %ebx           # Add 0x1000 to the B-register
	addl	$8, %edi                # Add eight to the desination index
	loop	_start.SetEntry

	# enable page address extension
	movl	%cr4, %eax
	orl		$(1 << 5), %eax
	movl	%eax, %cr4

	# enable long mode
	movl	$0xC0000080, %ecx
	rdmsr
	orl		$(1 << 8), %eax
	wrmsr

	# enable paging
	movl	%cr0, %eax
	orl		$(1 << 31), %eax
	movl	%eax, %cr0

	# load gdt
	lgdt	GDT.Pointer
	ljmp	$GDT.Code, $code64

	.code64
code64:

	movw	$GDT.Data, %dx        # set segment registers
	movw	%dx, %ds
	movw	%dx, %ss

	xorq	%rbp, %rbp      # set ebp to 0 so we know where to end stack traces

	pop		%rdi            # pop possible multiboot header
	pop		%rsi

	call	main
	cli

halt:
	hlt
	jmp		halt
