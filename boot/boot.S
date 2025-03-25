/**
** SCCS ID:	@(#)boot.S	2.4	1/22/25
**
** @file	boot.S
**
** @author	Jon Coles
**		copyleft 1999 Jon Coles
**
** @author	Warren R. Carithers, K. Reek, Garrett C. Smith
** @author	Walter Litwinczyk, David C. Larsen, Sean T. Congden
**
** Bootstrap routine.
**
** This bootstrap program is loaded by the PC BIOS into memory at
** location 0000:7C00.  It must be exactly 512 bytes long, and must
** end with the hex sequence AA55 at location 1FE.
**
** The bootstrap initially sets up a stack in low memory.  Next, it
** loads a second sector at 0000:7E00 (immediately following the
** boot block).  Then it loads the target program at TARGET_ADDR, 
** switches to protected mode, and branches to the target program.
**
** NOTE: To zero out the BSS segment, define CLEAR_BSS when this code
** is assembled.
**
** Must assemble this as 16-bit code.
*/
	.code16

#define ASM_SRC

#include <bootstrap.h>
#include <x86/bios.h>
#include <x86/arch.h>

/*
** Symbol for locating the beginning of the code.
*/
	.globl	bootentry

	.text
bootentry:

/*
** Entry point.  Disable interrupts and set up a runtime stack.
*/
	cli

	movw	$BOOT_SEG, %ax	/* data seg. base address */
	movw	%ax, %ds
	movw	%ax, %ss		/* also stack seg. base */
	movw	$BOOT_SP_DISP, %ax
	movw	%ax, %sp

/*
** Next, verify that the disk is there and working.
*/
	movb	$BD_CHECK, %ah /* test the disk status and make sure */
	movb	drive, %dl	 /* it's safe to proceed */
	int	$BIOS_DISK
	jnc	diskok

	movw	$err_diskstatus, %si /* Something went wrong; print a message */
	call	dispMsg		/* and freeze. */
	jmp	.

/*
** The disk is there. Reset it, and retrieve the disk parameters.
*/
diskok:
	movw	$BD_RESET, %ax	/* Reset the disk */
	movb	drive, %dl
	int	$BIOS_DISK

	/* determine number of heads and sectors/track */
	xorw	%ax, %ax    /* set ES:DI = 0000:0000 in case of BIOS bugs */
	movw	%ax, %es
	movw	%ax, %di
	movb	$BD_PARAMS, %ah	/* get drive parameters */
	movb	drive, %dl	/* hard disk or floppy */
	int	$BIOS_DISK

	/* store (max + 1) - CL[5:0] = maximum head, DH = maximum head */
	andb	$0x3F, %cl
	incb	%cl
	incb	%dh

	movb	%cl, max_sec
	movb	%dh, max_head

/*
** The disk is OK, so we now need to load the second half of the bootstrap.
** It must immediately follow the boot sector on the disk, and the target
** program(s) must immediately follow that.
*/
	movw	$msg_loading, %si /* Print the Loading message */
	call	dispMsg

	movw	$1, %ax			/* sector count = 1 */
	movw	$BOOT_SEG, %bx	/* read this into memory that */
	movw	%bx, %es		/* immediately follows this code. */
	movw	$PART2_DISP, %bx
	call	readprog

/*
** We've got the second block of the bootstrap program in memory. Now
** read all of the user's program blocks.  Use %di to point to the
** count field for the next block to load.
*/
	movw	$k_sect, %di

	pushw	%ds
	movw	(%di), %bx
	movw	$MMAP_SEG, %ax
	movw	%ax, %ds
	movw	%bx, MMAP_SECTORS	/* store kernel image size */
	popw	%ds

/*
** Each target program has three values in the array at the end of the
** second half of the bootstrap:  the offset and segment base address
** where the program should go, and the sector count.
*/
nextblock:
	movw	(%di), %ax	/* get the # of sectors */
	testw	%ax, %ax	/* is it zero? */
	jz	done_loading	/*   yes, nothing more to load. */

	subw	$2, %di
	movw	(%di), %bx	/* get the segment value */
	movw	%bx, %es	/*   and copy it to %es */
	subw	$2, %di
	movw	(%di), %bx	/* get the address offset */
	subw	$2, %di
	pushw	%di		/* save di */
	call	readprog	/* read this program block, */
	popw	%di		/* and restore di */
	jmp	nextblock	/*   then go back and read the next one. */

/*
** Read one complete program block into memory.
**
**	ax: number of sectors to read
**	es:bx = starting address for the block
*/
readprog:
	pushw	%ax		/* save sector count */

	movw	$3, %cx		/* initial retry count is 3 */
retry:
	pushw	%cx		/* push the retry count on the stack. */

	movw	sec, %cx	/* get sector number */
	movw	head, %dx	/* get head number */
	movb	drive, %dl

	movw	$BD_READ1, %ax	/* read 1 sector */
	int	$BIOS_DISK
	jnc	readcont	/* jmp if it worked ok */

	movw	$err_diskread, %si	/* report the error */
	call	dispMsg
	popw	%cx		/* get the retry count back */
	loop	retry		/*   and go try again. */
	movw	$err_diskfail, %si	/* can't proceed, */
	call	dispMsg		/* print message and freeze. */
	jmp	.

readcont:
	movw	$msg_dot, %si	/* print status: a dot */
	call	dispMsg
	cmpw	$OFFSET_LIMIT, %bx	/* have we reached the offset limit? */
	je	adjust		/* Yes--must adjust the es register */
	addw	$SECTOR_SIZE, %bx	/* No--just adjust the block size to */
	jmp	readcont2	/*    the offset and continue. */

adjust:
	movw	$0, %bx		/* start offset over again */
	movw	%es, %ax
	addw	$0x1000,%ax	/* move segment pointer to next chunk */
	movw	%ax, %es

readcont2:
	incb	%cl		/* not done - move to the next sector */
	cmpb	max_sec, %cl	/* see if we need */
	jnz	save_sector	/* to switch heads or tracks */

	movb	$1, %cl		/* reset sector number */
	incb	%dh		/* first, switch heads */
	cmpb	max_head, %dh	/* there are only two - if we've already */
	jnz	save_sector	/* used both, we need to switch tracks */

	xorb	%dh, %dh	/* reset to head 0 */
	incb	%ch		/* inc track number */
	cmpb	$80, %ch	/* 80 tracks per side - have we read all? */
	jnz	save_sector	/* read another track */

	movw	$err_toobig, %si 	/* report the error */
	call	dispMsg
	jmp	.		/* and freeze */

save_sector:
	movw	%cx, sec	/* save sector number */
	movw	%dx, head	/*   and head number */

	popw	%ax		/* discard the retry count */
	popw	%ax		/* get the sector count from the stack */
	decw	%ax		/*   and decrement it. */
	jg	readprog	/* If it is zero, we're done reading. */

readdone:
	movw	$msg_bar, %si	/* print message saying this block is done */
	call	dispMsg
	ret			/* and return to the caller */

/*
** We've loaded the whole target program into memory,
** so it's time to transfer to the startup code.
*/
done_loading:
	movw	$msg_go, %si	/* last status message */
	call	dispMsg

	jmp	switch		/* move to the next phase */
	
/*
** Support routine - display a message byte by byte to the monitor.
*/
dispMsg:	
	pushw	%ax
	pushw	%bx
repeat:
	lodsb			/* grab next character */

	movb	$BV_W_ADV, %ah	/* write and advance cursor */
	movw	$0x07, %bx	/* page 0, white on blank, no blink */
	orb	%al, %al	/* AL is character to write */
	jz	getOut		/* if we've reached the NUL, get out */

	int	$BIOS_VIDEO	/* otherwise, print and repeat */
	jmp	repeat	

getOut:				/* we're done, so return */
	popw	%bx
	popw	%ax
	ret

/*
** Support routine - move the GDT entries from where they are to
** location 0050:0000. We need to add BOOT_ADDR because the bootstrap
** is linked at 0, but loaded at 0x7c00.
*/
move_gdt:
	movw	%cs, %si
	movw	%si, %ds
	movw	$start_gdt + BOOT_ADDR, %si
	movw	$GDT_SEG, %di
	movw	%di, %es
	xorw	%di, %di
	movl	$gdt_len, %ecx
	cld
	rep	movsb
	ret

/*
** DATA AREAS.
**
** Next sector number and head number to read from.
*/
sec:		.word	2	/* cylinder=0, sector=1 */
head:		.word	0	/* head=0 */
max_sec:	.byte	19	/* up to 18 sectors per floppy track */
max_head:	.byte	2	/* only two r/w heads per floppy drive */

/*
** Status and error messages.
*/
msg_loading:	.asciz "Loading"
msg_dot:	.asciz "."
msg_go:		.asciz "done."
msg_bar:	.asciz	"|"

/*
** Error messages.
*/
err_diskstatus:	.asciz "Disk not ready.\n\r"
err_diskread:	.asciz "Read failed\n\r"
err_toobig:	.asciz	"Too big\n\r"
err_diskfail:	.asciz	"Can't proceed\n\r"

/*
** Data areas.
*/

/*
** The GDTR and IDTR contents.
*/
gdt_48:
	.word	0x2000		/* 1024 GDT entries x 8 bytes/entry = 8192 */
	.quad	GDT_ADDR

idt_48:
	.word	0x0800		/* 256 interrupts */
	.quad	IDT_ADDR

/*
** Depending on the age of the BIOS, it may expect there to be a
** partition table for the hard drive you're booting from at this point
** in the boot sector; only the first 446 bytes (0x000-0x1bd) can be
** used for bootstrap code/data.  To make life easy, we'll just skip
** over the rest of the sector.
**
** Note: when booting from floppy, this isn't a problem, because floppy
** disks don't have partition tables.  On some machines, USB-type storage
** devices are treated as floppies, so they also don't have partition
** maps; however, on other systems, USB storage is treated as hard disk
** storage.
*/

/*
** End of the first sector of the boot program.  The last two bytes
** of this sector must be AA55 in order for the disk to be recognized
** by the BIOS as bootable.
*/
	.org	SECTOR_SIZE-4

drive:	.word	BDEV	/* 0x00 = floppy, 0x80 = usb */

boot_sig:
	.word 0xAA55

/*******************************************************
******* BEGINNING OF SECTOR TWO OF THE BOOTSTRAP *******
*******************************************************/

/*
** This code configures the GDT, enters protected mode, and then
** transfers to the OS entry point.
*/

switch:
	cli
	movb	$NMI_DISABLE, %al	/* also disable NMIs */
	outb	%al, $CMOS_ADDR

#ifdef USE_FLOPPY
	call	floppy_off
#endif
	call	enable_A20
	call	move_gdt
#ifdef GET_MMAP
	call	check_memory
#endif

/*
** Get the memory address for the "user blob" out of the table
** at the end of this sector, and pass the three values to the
** protected mode code in %bx, %cx, and %dx. We could figure out
** how to find it from there, but this is easier.
*/
#	movw	u_off+BOOT_ADDR, %bx
#	movw	u_seg+BOOT_ADDR, %cx
#	movw	u_sect+BOOT_ADDR, %dx

/*
** The IDTR and GDTR are loaded relative to this segment, so we must
** use the full offsets from the beginning of the segment (0000:0000);
** however, we were loaded at 0000:7c00, so we need to add that in.
*/
	lidt	idt_48 + BOOT_ADDR
	lgdt	gdt_48 + BOOT_ADDR

	movl	%cr0, %eax	/* get current CR0 */
	orl	$1, %eax	/* set the PE bit */
	movl	%eax, %cr0	/* and store it back. */
	
	/*
	** We'll be in protected mode at the start of the user's code
	** right after this jump executes.
	**
	** First, a byte to force 32-bit mode execution, followed by
	** a 32-bit long jump.  The long ("far") jump loads both EIP
	** and CS with the proper values so that when we land at the
	** destination address in protected mode, the next instruction
	** fetch doesn't cause a fault.
	**
	** The old code for this:
	**
	**	.byte	0x66, 0xEA
	**	.long	TARGET_ADDR
	**	.word	GDT_CODE
	*/

	.byte	0x66
	.code32
	ljmp	$GDT_CODE, $TARGET_ADDR
	.code16

/*
** Supporting functions.
*/

#ifdef USE_FLOPPY
/*
** Turn off the motor on the floppy disk drive.
*/
floppy_off:
	push	%dx
	movw	$0x3f2, %dx
	xorb	%al, %al
	outb	%al, %dx
	pop	%dx
	ret
#endif

/*
** Enable the A20 gate for full memory access.
*/
enable_A20:
	call	a20wait
	movb	$KBD_P1_DISABLE, %al
	outb	%al, $KBD_CMD

	call	a20wait
	movb	$KBD_RD_OPORT, %al
	outb	%al, $KBD_CMD

	call	a20wait2
	inb	$KBD_DATA, %al
	pushl	%eax

	call	a20wait
	movb	$KBD_WT_OPORT, %al
	outb	%al, $KBD_CMD

	call	a20wait
	popl	%eax
	orb	$2, %al
	outb	%al, $KBD_DATA

	call	a20wait
	mov	$KBD_P1_ENABLE, %al
	out	%al, $KBD_CMD

	call	a20wait
	ret

a20wait:	/* wait until bit 1 of the device register is clear */
	movl    $65536, %ecx	/* loop a lot if need be */
wait_loop: 
	inb     $KBD_STAT, %al	/* grab the byte */
	test    $2, %al		/* is the bit clear? */
	jz      wait_exit	/* yes */
	loop    wait_loop	/* no, so loop */
	jmp     a20wait		/* if still not clear, go again */
wait_exit:    
	ret

a20wait2:	/* like a20wait, but waits until bit 0 is set. */
	mov     $65536, %ecx
wait2_loop:
	in      $KBD_STAT, %al
	test    $1, %al
	jnz     wait2_exit
	loop    wait2_loop
	jmp     a20wait2
wait2_exit:
	ret

#ifdef GET_MMAP
/*
** Query the BIOS to get the list of usable memory regions
**
** Adapted from: http://wiki.osdev.org/Detecting_Memory_%28x86%29
** (see section "BIOS Function INT 0x15. EAX = 0xE820")
**
** After the first 'int', if the location 0x2D00 (4 bytes) contains -1,
** then this method failed to detect memory properly; otherwise, this
** location contains the number of elements read.
**
** The start of the array is at 0x2D04. The elements are tightly
** packed following the layout as defined below.  Each entry in the
** array contains the following information:
**
**	uint64_t  base address of region
**	uint64_t  length of region (0 --> ignore the entry)
**	uint32_t  type of region
**	uint32_t  ACIP 3.0 Extended Attributes
**
** The C struct definition is as follows:
**
** struct MemMapEntry
** {
**    uint32_t base[2];    // 64-bit base address
**    uint32_t length[2];  // 64-bit length
**    uint32_t type;       // 32-bit region type
**    uint32_t ACPI;       // 32-bit ACPI "extended attributes" bitfield
** };
**
** This structure must be packed in memory.  This shouldn't be a problem,
** but if it is, you may need to add this attribute at the end of the
** struct declaration before the semicolon:
**
**    __attribute__((packed))
**
** Parameters:
**     None
**/
check_memory:
	// save everything
	// pushaw won't work here because we're still in real mode
	pushw	%ds
	pushw	%es
	pushw	%ax
	pushw	%bx
	pushw	%cx
	pushw	%dx
	pushw	%si
	pushw	%di

	// Set the start of the buffer
	movw	$MMAP_SEG, %bx // 0x2D0
	mov	%bx, %ds	// Data segment now starts at 0x2D00
	mov	%bx, %es	// Extended segment also starts at 0x2D00

	// Reserve the first 4 bytes for the # of entries
	movw	$0x4, %di
	// Make a valid ACPI 3.X entry
	movw	$1, %es:20(%di)

	xorw	%bp, %bp	// Count of entries in the list
	xorl	%ebx, %ebx	// EBX must contain zeroes

	movl	$MMAP_MAGIC_NUM, %edx	// Magic number into EDX
	movl	$MMAP_CODE, %eax	// E820 memory command
	movl	$MMAP_ENT, %ecx	// Ask the BIOS for 24 bytes
	int	$BIOS_MISC	// Call the BIOS

	// check for success
	jc	cm_failed	// C == 1 --> failure
	movl	$MMAP_MAGIC_NUM, %edx	// sometimes EDX changes
	cmpl	%eax, %edx	// EAX should equal EDX after the call
	jne	cm_failed
	testl	%ebx, %ebx	// Should have at least one more entry
	je	cm_failed

	jmp	cm_jumpin	// Good to go - start us off

cm_loop:
	movl	$MMAP_CODE, %eax	// Reset our registers
	movw	$1, 20(%di)
	movl	$MMAP_ENT, %ecx
	int	$BIOS_MISC
	jc	cm_end_of_list	// C == 1 --> end of list
	movl	$MMAP_MAGIC_NUM, %edx

cm_jumpin:
	jcxz	cm_skip_entry	// Did we get any data?

	cmp	$20, %cl	// Check the byte count
	jbe	cm_no_text	// Skip the next test if only 20 bytes

	testb	$1, %es:20(%di) // Check the "ignore this entry" flag
	je	cm_skip_entry

cm_no_text:
	mov	%es:8(%di), %ecx	// lower half of length
	or	%es:12(%di), %ecx	// now, full length
	jz	cm_skip_entry

	inc	%bp		// one more valid entry

	// make sure we don't overflow our space
	cmpw	$MMAP_MAX_ENTS, %bp
	jge	cm_end_of_list

	// we're ok - move the pointer to the next struct in the array
	add	$24, %di

cm_skip_entry:
	// are there more entries to retrieve?
	testl	%ebx, %ebx
	jne	cm_loop

cm_end_of_list:
	// All done!  Store the number of elements in 0x2D00
	movw	%bp, %ds:0x0

	clc	// Clear the carry bit and return
	jmp	cm_ret

cm_failed:
	movl	$-1, %ds:0x0	// indicate failure
	stc

cm_ret:
	// restore everything we saved
	// popaw won't work here (still in real mode!)
	popw	%di
	popw	%si
	popw	%dx
	popw	%cx
	popw	%bx
	popw	%ax
	popw	%es
	popw	%ds
	ret
#endif

/*
** The GDT.  This cannot be created in C because the bootstrap is not
** linked with that code.  We could just have a simple "dummy" GDT here
** but that would only save us a couple of entries. Also, we could save
** some space by not having the separate 'linear' and 'stack' entries
** (they're identical to the 'data' entry).
*/
	.p2align 2	// force 4-byte alignment
start_gdt:
	// selector 0x0000 is unused
	SEGNULL

	// selector 0x0008 - basic linear access to all of memory
	SEGMENT( 0x0, 0xffffffff, SEG_DPL_0, SEG_DATA_RW )

	// selector 0x0010 - kernel code segment
	SEGMENT( 0x0, 0xffffffff, SEG_DPL_0, SEG_CODE_XR )

	// selector 0x0018 - kernel data segment
	SEGMENT( 0x0, 0xffffffff, SEG_DPL_0, SEG_DATA_RW )

	// selector 0x0020 - kernel stack segment
	SEGMENT( 0x0, 0xffffffff, SEG_DPL_0, SEG_DATA_RW )

	// could put additional entries here for user mode - e.g.,
	// 0x0028 code:  SEGMENT( 0x0, 0xffffffff, SEG_DPL_3, SEG_CODE_XR )
	// 0x0030 data:  SEGMENT( 0x0, 0xffffffff, SEG_DPL_3, SEG_DATA_RW )
	// 0x0038 stack: SEGMENT( 0x0, 0xffffffff, SEG_DPL_3, SEG_DATA_RW )

end_gdt:
gdt_len = end_gdt - start_gdt

/*
** The end of this program will contain a list of the sizes and load
** addresses of all of the blocks to be loaded.  These values are
** inserted here by the BuildImage program, which checks that there are
** not so many blocks that the GDT would be overwritten.  The layout
** of the data is:
**
**	struct info_s {
**	    short offset;
**	    short segment;
**	    short sectors;
**	};
**
** with the data for the first program at k_off, k_seg, and k_sect.
** If additional blocks are to be loaded, their values appear just
** before the previous set.
*/

	.org	BOOT_SIZE-12
u_off:	.word	0	// the "user blob"
u_seg:	.word	0
u_sect:	.word	0
k_off:	.word	0	// the kernel
k_seg:	.word	0
k_sect:	.word	0
