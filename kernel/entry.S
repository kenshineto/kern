#include <comus/limits.h>

	.globl _start
	.globl kernel_pml4
	.globl kernel_pdpt_0
	.globl kernel_pd_0
	.globl kernel_pd_0_ents
	.globl kernel_pd_1
	.globl paging_pt
	.globl GDT
	.globl kern_stack_end
	.globl kern_stack_start
	.extern main

	.section .multiboot

	# multiboot header
mb_start:
	# magic
	.align 8
	.long 0xe85250d6
	.long 0
	.long mb_end - mb_start
	.long 0x100000000 - (0xe85250d6 + (mb_end - mb_start))
	# info request
	.align 8
	.short 1
	.short 1
	.long 36
	.long 1 # cmdline
	.long 3 # module
	.long 6 # mmap
	.long 9 # elf section
	.long 12 # efi64
	.long 14 # rsdp
	.long 15 # xsdp
	.long 20 # efi64 IH
	# bios entry
	.align 8
	.short 3
	.short 0
	.long 12
	.long _start
	# framebuffer
	.align 8
	.short 5
	.short 1
	.long 20
	.long 720 # width
	.long 480 # height
	.long 32 # bpp
	# module align
	.align 8
	.short 6
	.short 0
	.long 8
	# efi boot services
	.align 8
	.short 7
	.short 0
	.long 8
	# efi amd64 entry
	.align 8
	.short 9
	.short 0
	.long 12
	.long _start_efi
	# null
	.align 8
	.short 0
	.short 0
	.long 8
mb_end:

	.section .bss

	# kernel page tables
	.align 4096
kernel_pml4:
	.skip 4096
kernel_pdpt_0:
	.skip 4096
kernel_pd_0:
	.skip 4096
kernel_pd_0_ents:
	.skip (4096*N_IDENT_PTS)
kernel_pd_1:
	.skip 4096
paging_pt:
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
	.quad 0
	.quad 0 # to be modified in kernel

	# GDT Pointer
	.equ GDT.Pointer, .
	.word . - GDT - 1
	.quad GDT

	.section .text
	.code32

_start:
    # disable interrupts
	cli

    # setup stack
	movl	$kern_stack_end, %esp
	movl	$kern_stack_end, %ebp

	# save multiboot (if using multiboot)
	pushl	$0
	push 	%ebx
	pushl	$0
	push 	%eax

	# setup kernel paging structures
	movl	$kernel_pml4, %edi             # zero out pml4
	movl	%edi, %cr3
	xorl	%eax, %eax
	movl	$0x1000, %ecx
	rep		stosl
	movl	%cr3, %edi

	movl	$kernel_pdpt_0 + 3, (%edi)     # map pdpt
	movl	$kernel_pdpt_0, %edi

	movl	$kernel_pd_0 + 3, (%edi)       # map pd 0

	addl	$8, %edi                       # map pd 1
	movl	$kernel_pd_1 + 3, (%edi)

	movl	$kernel_pd_0, %edi             # map pd 0 ents
	movl	$kernel_pd_0_ents + 3, %ebx
	movl	$N_IDENT_PTS, %ecx
_start.map_pd_0:
	movl	%ebx, (%edi)
	addl	$(8 * 512), %ebx
	addl	$8, %edi
	loop	_start.map_pd_0

	movl	$kernel_pd_0_ents, %edi        # identity map kernel
	movl	$0x03, %ebx
	movl	$(512 * N_IDENT_PTS), %ecx
_start.map_pd_0_ents:
	movl	%ebx, (%edi)
	addl	$0x1000, %ebx
	addl	$8, %edi
	loop	_start.map_pd_0_ents

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
_start_efi:
    # disable interrupts
	cli

	# setup stack
	movq	$kern_stack_end, %rsp
	movq	$kern_stack_end, %rbp

	# save multiboot
	pushq	%rbx
	pushq	%rax

	# load gdt
	lgdt	GDT.Pointer
	pushq	$GDT.Code
	pushq	$code64
	retfq

code64:
	# set segment registers
	movw	$GDT.Data, %dx
	movw	%dx, %ds
	movw	%dx, %es
	movw	%dx, %fs
	movw	%dx, %gs
	movw	%dx, %ss

	# set ebp to 0 so we know where to end stack traces
	xorq	%rbp, %rbp

	# pop multiboot header
	pop		%rdi
	pop		%rsi

	call	main

halt:
	cli
	hlt
	jmp		halt
