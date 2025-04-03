/*
** @file	isrs.S
**
** @author	K. Reek
** @authors	Jon Coles, Warren R. Carithers, Margaret Reek
** @author	numerous Systems Programming classes
**
** Stubs for ISRs.
**
** This module provides the stubs needed for interrupts to save
** the machine state before calling the ISR.  All interrupts have
** their own stub which pushes the interrupt number on the stack.
** This makes it possible for a common ISR to determine which
** interrupted occurred.
*/

#define ASM_SRC

#	.arch	i386

#include <bootstrap.h>
#include <offsets.h>
#include <vm.h>

/*
** Configuration options - define in Makefile
**
**	TRACE_CX	include context restore debugging code
*/

	.text

/*
** Macros for the isr stubs.  Some interrupts push an error code on
** the stack and others don't; for those that don't we simply push
** a zero so that cleaning up from either type is identical.
**
** Note: these are not marked as global symbols, as they are never
** accessed directly outside of this file.  This could be changed
** if need be by adding this line to each macro definition right
** after the #define line:
**
**	.global isr_##vector
*/

#define	ISR(vector)			\
isr_##vector:				; \
	pushl	$0			; \
	pushl	$vector			; \
	jmp	isr_save

#define	ERR_ISR(vector)		\
isr_##vector:				; \
	pushl	$vector			; \
	jmp	isr_save

	.globl	isr_table
	.globl	isr_restore

/*
** This routine saves the machine state, calls the ISR, and then
** restores the machine state and returns from the interrupt.
**
********************************************************************
********************************************************************
** NOTE:  this code is highly application-specific, and will most **
** probably require modification to tailor it.                    **
**                                                                **
** Examples of mods:  switch to/from user stack, context switch   **
** changes, etc.                                                  **
********************************************************************
********************************************************************
*/

isr_save:

/*
** Begin by saving the CPU state (except for the FP context information).
**
** At this point, the stack looks like this:
**
**  esp ->  vector #		saved by the entry macro
**	    error code, or 0	saved by the hardware, or the entry macro
**	    saved EIP		saved by the hardware
**	    saved CS		saved by the hardware
**	    saved EFLAGS	saved by the hardware
*/
	pusha			// save E*X, ESP, EBP, ESI, EDI
	pushl	%ds		// save segment registers
	pushl	%es
	pushl	%fs
	pushl	%gs
	pushl	%ss

/*
** Stack contents (all 32-bit longwords) and offsets from ESP:
**
**   SS GS FS ES DS EDI ESI EBP ESP EBX EDX ECX EAX vec cod EIP CS EFL
**   0  4  8  12 16 20  24  28  32  36  40  44  48  52  56  60  64 68
**
** Note that the saved ESP is the contents before the PUSHA.
**
** Set up parameters for the ISR call.
*/
	movl	CTX_vector(%esp),%eax	// get vector number and error code
	movl	CTX_code(%esp),%ebx

/*
***********************
** MOD FOR 20245     **
***********************
*/

/*
** We need to switch to the system stack. This requires that we save
** the user context pointer into the current PCB, then load ESP with
** the initial system stack pointer.
*/

	.globl	current
	.globl	kernel_esp

	// save the context pointer
	movl	current, %edx
	movl	%esp, PCB_context(%edx)

	// also save the page directory pointer
	movl	%cr3, %ecx
	addl	$KERN_BASE, %ecx	// convert to a virtual address
	movl	%ecx, PCB_pdir(%edx)

	// switch to the system stack
	//
	// NOTE: this is inherently non-reentrant!  If/when the OS
	// is converted from monolithic to something that supports
	// reentrant or interruptable ISRs, this code will need to
	// be changed to support that!

	movl	kernel_esp, %esp

	// we don't change CR3 because all the user PDIRs are
	// set up with mappings for the OS in the upper half

/*
***********************
** END MOD FOR 20245 **
***********************
*/

	pushl	%ebx		// put them on the top of the stack ...
	pushl	%eax		// ... as parameters for the ISR

/*
** Call the ISR
*/
	movl	isr_table(,%eax,4),%ebx
	call	*%ebx
	addl	$8,%esp		// pop the two parameters

/*
** Context restore begins here
*/

isr_restore:

/*
***********************
** MOD FOR 20245     **
***********************
*/
	movl	current, %ebx	// return to the user stack
	movl	PCB_context(%ebx), %esp	// ESP --> context save area
	movl	PCB_pdir(%ebx), %ecx	// page directory pointer
	subl	$KERN_BASE, %ecx	// convert to a physical address
	movl	%ecx, %cr3

	// now we're operating with the user process'
	// page directory and stack

/*
***********************
** END MOD FOR 20245 **
***********************
*/

#ifdef TRACE_CX
/*
** DEBUGGING CODE PART 1
**
** This code will execute during each context restore, and
** should be modified to print out whatever debugging information
** is desired.
**
** By default, it prints out the CPU context being restored; it
** relies on the standard save sequence (see above).
*/
	.globl	cio_printf_at

	pushl	$fmt
	pushl	$1
	pushl	$0
	call	cio_printf_at
	addl	$12,%esp
/*
** END OF DEBUGGING CODE PART 1
*/
#endif

/*
** Restore the context.
*/
	popl	%ss		// restore the segment registers
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds
	popa			// restore others
	addl	$8, %esp	// discard the error code and vector
	iret			// and return

#ifdef TRACE_CX
/*
** DEBUGGING CODE PART 2
**
** This format string is arranged according to the ordering of values
** in the context save area on the stack.
*/
fmt:	.ascii	" ss=%08x  gs=%08x  fs=%08x  es=%08x  ds=%08x\n"
	.ascii	"edi=%08x esi=%08x ebp=%08x esp=%08x ebx=%08x\n"
	.ascii	"edx=%08x ecx=%08x eax=%08x vec=%08x cod=%08x\n"
	.string	"eip=%08x  cs=%08x efl=%08x\n"

/*
** END OF DEBUGGING CODE PART 2
*/
#endif

/*
** Here we generate the individual stubs for each interrupt.
*/
ISR(0x00);	ISR(0x01);	ISR(0x02);	ISR(0x03);
ISR(0x04);	ISR(0x05);	ISR(0x06);	ISR(0x07);
ERR_ISR(0x08);	ISR(0x09);	ERR_ISR(0x0a);	ERR_ISR(0x0b);
ERR_ISR(0x0c);	ERR_ISR(0x0d);	ERR_ISR(0x0e);	ISR(0x0f);
ISR(0x10);	ERR_ISR(0x11);	ISR(0x12);	ISR(0x13);
ISR(0x14);	ERR_ISR(0x15);	ISR(0x16);	ISR(0x17);
ISR(0x18);	ISR(0x19);	ISR(0x1a);	ISR(0x1b);
ISR(0x1c);	ISR(0x1d);	ISR(0x1e);	ISR(0x1f);
ISR(0x20);	ISR(0x21);	ISR(0x22);	ISR(0x23);
ISR(0x24);	ISR(0x25);	ISR(0x26);	ISR(0x27);
ISR(0x28);	ISR(0x29);	ISR(0x2a);	ISR(0x2b);
ISR(0x2c);	ISR(0x2d);	ISR(0x2e);	ISR(0x2f);
ISR(0x30);	ISR(0x31);	ISR(0x32);	ISR(0x33);
ISR(0x34);	ISR(0x35);	ISR(0x36);	ISR(0x37);
ISR(0x38);	ISR(0x39);	ISR(0x3a);	ISR(0x3b);
ISR(0x3c);	ISR(0x3d);	ISR(0x3e);	ISR(0x3f);
ISR(0x40);	ISR(0x41);	ISR(0x42);	ISR(0x43);
ISR(0x44);	ISR(0x45);	ISR(0x46);	ISR(0x47);
ISR(0x48);	ISR(0x49);	ISR(0x4a);	ISR(0x4b);
ISR(0x4c);	ISR(0x4d);	ISR(0x4e);	ISR(0x4f);
ISR(0x50);	ISR(0x51);	ISR(0x52);	ISR(0x53);
ISR(0x54);	ISR(0x55);	ISR(0x56);	ISR(0x57);
ISR(0x58);	ISR(0x59);	ISR(0x5a);	ISR(0x5b);
ISR(0x5c);	ISR(0x5d);	ISR(0x5e);	ISR(0x5f);
ISR(0x60);	ISR(0x61);	ISR(0x62);	ISR(0x63);
ISR(0x64);	ISR(0x65);	ISR(0x66);	ISR(0x67);
ISR(0x68);	ISR(0x69);	ISR(0x6a);	ISR(0x6b);
ISR(0x6c);	ISR(0x6d);	ISR(0x6e);	ISR(0x6f);
ISR(0x70);	ISR(0x71);	ISR(0x72);	ISR(0x73);
ISR(0x74);	ISR(0x75);	ISR(0x76);	ISR(0x77);
ISR(0x78);	ISR(0x79);	ISR(0x7a);	ISR(0x7b);
ISR(0x7c);	ISR(0x7d);	ISR(0x7e);	ISR(0x7f);
ISR(0x80);	ISR(0x81);	ISR(0x82);	ISR(0x83);
ISR(0x84);	ISR(0x85);	ISR(0x86);	ISR(0x87);
ISR(0x88);	ISR(0x89);	ISR(0x8a);	ISR(0x8b);
ISR(0x8c);	ISR(0x8d);	ISR(0x8e);	ISR(0x8f);
ISR(0x90);	ISR(0x91);	ISR(0x92);	ISR(0x93);
ISR(0x94);	ISR(0x95);	ISR(0x96);	ISR(0x97);
ISR(0x98);	ISR(0x99);	ISR(0x9a);	ISR(0x9b);
ISR(0x9c);	ISR(0x9d);	ISR(0x9e);	ISR(0x9f);
ISR(0xa0);	ISR(0xa1);	ISR(0xa2);	ISR(0xa3);
ISR(0xa4);	ISR(0xa5);	ISR(0xa6);	ISR(0xa7);
ISR(0xa8);	ISR(0xa9);	ISR(0xaa);	ISR(0xab);
ISR(0xac);	ISR(0xad);	ISR(0xae);	ISR(0xaf);
ISR(0xb0);	ISR(0xb1);	ISR(0xb2);	ISR(0xb3);
ISR(0xb4);	ISR(0xb5);	ISR(0xb6);	ISR(0xb7);
ISR(0xb8);	ISR(0xb9);	ISR(0xba);	ISR(0xbb);
ISR(0xbc);	ISR(0xbd);	ISR(0xbe);	ISR(0xbf);
ISR(0xc0);	ISR(0xc1);	ISR(0xc2);	ISR(0xc3);
ISR(0xc4);	ISR(0xc5);	ISR(0xc6);	ISR(0xc7);
ISR(0xc8);	ISR(0xc9);	ISR(0xca);	ISR(0xcb);
ISR(0xcc);	ISR(0xcd);	ISR(0xce);	ISR(0xcf);
ISR(0xd0);	ISR(0xd1);	ISR(0xd2);	ISR(0xd3);
ISR(0xd4);	ISR(0xd5);	ISR(0xd6);	ISR(0xd7);
ISR(0xd8);	ISR(0xd9);	ISR(0xda);	ISR(0xdb);
ISR(0xdc);	ISR(0xdd);	ISR(0xde);	ISR(0xdf);
ISR(0xe0);	ISR(0xe1);	ISR(0xe2);	ISR(0xe3);
ISR(0xe4);	ISR(0xe5);	ISR(0xe6);	ISR(0xe7);
ISR(0xe8);	ISR(0xe9);	ISR(0xea);	ISR(0xeb);
ISR(0xec);	ISR(0xed);	ISR(0xee);	ISR(0xef);
ISR(0xf0);	ISR(0xf1);	ISR(0xf2);	ISR(0xf3);
ISR(0xf4);	ISR(0xf5);	ISR(0xf6);	ISR(0xf7);
ISR(0xf8);	ISR(0xf9);	ISR(0xfa);	ISR(0xfb);
ISR(0xfc);	ISR(0xfd);	ISR(0xfe);	ISR(0xff);

	.data

/*
** This table contains the addresses where each of the preceding
** stubs begins.  This information is needed to initialize the
** Interrupt Descriptor Table in support.c
*/
	.globl	isr_stub_table
isr_stub_table:
	.long	isr_0x00, isr_0x01, isr_0x02, isr_0x03
	.long	isr_0x04, isr_0x05, isr_0x06, isr_0x07
	.long	isr_0x08, isr_0x09, isr_0x0a, isr_0x0b
	.long	isr_0x0c, isr_0x0d, isr_0x0e, isr_0x0f
	.long	isr_0x10, isr_0x11, isr_0x12, isr_0x13
	.long	isr_0x14, isr_0x15, isr_0x16, isr_0x17
	.long	isr_0x18, isr_0x19, isr_0x1a, isr_0x1b
	.long	isr_0x1c, isr_0x1d, isr_0x1e, isr_0x1f
	.long	isr_0x20, isr_0x21, isr_0x22, isr_0x23
	.long	isr_0x24, isr_0x25, isr_0x26, isr_0x27
	.long	isr_0x28, isr_0x29, isr_0x2a, isr_0x2b
	.long	isr_0x2c, isr_0x2d, isr_0x2e, isr_0x2f
	.long	isr_0x30, isr_0x31, isr_0x32, isr_0x33
	.long	isr_0x34, isr_0x35, isr_0x36, isr_0x37
	.long	isr_0x38, isr_0x39, isr_0x3a, isr_0x3b
	.long	isr_0x3c, isr_0x3d, isr_0x3e, isr_0x3f
	.long	isr_0x40, isr_0x41, isr_0x42, isr_0x43
	.long	isr_0x44, isr_0x45, isr_0x46, isr_0x47
	.long	isr_0x48, isr_0x49, isr_0x4a, isr_0x4b
	.long	isr_0x4c, isr_0x4d, isr_0x4e, isr_0x4f
	.long	isr_0x50, isr_0x51, isr_0x52, isr_0x53
	.long	isr_0x54, isr_0x55, isr_0x56, isr_0x57
	.long	isr_0x58, isr_0x59, isr_0x5a, isr_0x5b
	.long	isr_0x5c, isr_0x5d, isr_0x5e, isr_0x5f
	.long	isr_0x60, isr_0x61, isr_0x62, isr_0x63
	.long	isr_0x64, isr_0x65, isr_0x66, isr_0x67
	.long	isr_0x68, isr_0x69, isr_0x6a, isr_0x6b
	.long	isr_0x6c, isr_0x6d, isr_0x6e, isr_0x6f
	.long	isr_0x70, isr_0x71, isr_0x72, isr_0x73
	.long	isr_0x74, isr_0x75, isr_0x76, isr_0x77
	.long	isr_0x78, isr_0x79, isr_0x7a, isr_0x7b
	.long	isr_0x7c, isr_0x7d, isr_0x7e, isr_0x7f
	.long	isr_0x80, isr_0x81, isr_0x82, isr_0x83
	.long	isr_0x84, isr_0x85, isr_0x86, isr_0x87
	.long	isr_0x88, isr_0x89, isr_0x8a, isr_0x8b
	.long	isr_0x8c, isr_0x8d, isr_0x8e, isr_0x8f
	.long	isr_0x90, isr_0x91, isr_0x92, isr_0x93
	.long	isr_0x94, isr_0x95, isr_0x96, isr_0x97
	.long	isr_0x98, isr_0x99, isr_0x9a, isr_0x9b
	.long	isr_0x9c, isr_0x9d, isr_0x9e, isr_0x9f
	.long	isr_0xa0, isr_0xa1, isr_0xa2, isr_0xa3
	.long	isr_0xa4, isr_0xa5, isr_0xa6, isr_0xa7
	.long	isr_0xa8, isr_0xa9, isr_0xaa, isr_0xab
	.long	isr_0xac, isr_0xad, isr_0xae, isr_0xaf
	.long	isr_0xb0, isr_0xb1, isr_0xb2, isr_0xb3
	.long	isr_0xb4, isr_0xb5, isr_0xb6, isr_0xb7
	.long	isr_0xb8, isr_0xb9, isr_0xba, isr_0xbb
	.long	isr_0xbc, isr_0xbd, isr_0xbe, isr_0xbf
	.long	isr_0xc0, isr_0xc1, isr_0xc2, isr_0xc3
	.long	isr_0xc4, isr_0xc5, isr_0xc6, isr_0xc7
	.long	isr_0xc8, isr_0xc9, isr_0xca, isr_0xcb
	.long	isr_0xcc, isr_0xcd, isr_0xce, isr_0xcf
	.long	isr_0xd0, isr_0xd1, isr_0xd2, isr_0xd3
	.long	isr_0xd4, isr_0xd5, isr_0xd6, isr_0xd7
	.long	isr_0xd8, isr_0xd9, isr_0xda, isr_0xdb
	.long	isr_0xdc, isr_0xdd, isr_0xde, isr_0xdf
	.long	isr_0xe0, isr_0xe1, isr_0xe2, isr_0xe3
	.long	isr_0xe4, isr_0xe5, isr_0xe6, isr_0xe7
	.long	isr_0xe8, isr_0xe9, isr_0xea, isr_0xeb
	.long	isr_0xec, isr_0xed, isr_0xee, isr_0xef
	.long	isr_0xf0, isr_0xf1, isr_0xf2, isr_0xf3
	.long	isr_0xf4, isr_0xf5, isr_0xf6, isr_0xf7
	.long	isr_0xf8, isr_0xf9, isr_0xfa, isr_0xfb
	.long	isr_0xfc, isr_0xfd, isr_0xfe, isr_0xff
