/*
** SCCS ID:	@(#)bootstrap.h	2.4	1/22/25
**
** @file	bootstrap.h
**
** @author	K. Reek
** @author	Warren R. Carithers, Garrett C. Smith
**
** Addresses where various stuff goes in memory.
*/

#ifndef	BOOTSTRAP_H_
#define	BOOTSTRAP_H_

/*
** The boot device
*/
#define	BDEV_FLOPPY		0x00
#define	BDEV_USB		0x80	/* hard drive */

#define	BDEV			BDEV_USB	/* default */

/*
** Bootstrap definitions
*/
#define	BOOT_SEG		0x07c0		/* 07c0:0000 */
#define	BOOT_DISP		0x0000
#define	BOOT_ADDR		((BOOT_SEG << 4) + BOOT_DISP)

#define	PART2_DISP		0x0200		/* 07c0:0200 */
#define	PART2_ADDR		((BOOT_SEG << 4) + PART2_DISP)

#define	SECTOR_SIZE		0x200	/* 512 bytes */

/* Note: this assumes the bootstrap is two sectors long! */
#define	BOOT_SIZE		(SECTOR_SIZE + SECTOR_SIZE)

#define	OFFSET_LIMIT	(0x10000 - SECTOR_SIZE)

#define	BOOT_SP_DISP	0x4000	/* stack pointer 07c0:4000, or 0xbc00 */
#define	BOOT_SP_ADDR	((BOOT_SEG << 4) + BOOT_SP_DISP)

#define SECTOR1_END		(BOOT_ADDR + SECTOR_SIZE)
#define SECTOR2_END		(BOOT_ADDR + BOOT_SIZE)

// location of the user blob data
// (three halfwords of data)
#define	USER_BLOB_DATA  (SECTOR2_END - 12)

/*
** The target program itself
*/
#define	TARGET_SEG		0x00001000		/* 1000:0000 */
#define	TARGET_ADDR		0x00010000		/* and upward */
#define	TARGET_STACK	0x00010000		/* and downward */

/*
** The Global Descriptor Table (0000:0500 - 0000:2500)
*/
#define	GDT_SEG			0x00000050
#define	GDT_ADDR		0x00000500

	/* segment register values */
#define	GDT_LINEAR		0x0008		/* All of memory, R/W */
#define	GDT_CODE		0x0010		/* All of memory, R/E */
#define	GDT_DATA		0x0018		/* All of memory, R/W */
#define	GDT_STACK		0x0020		/* All of memory, R/W */

/*
** The Interrupt Descriptor Table (0000:2500 - 0000:2D00)
*/
#define	IDT_SEG			0x00000250
#define	IDT_ADDR		0x00002500

/*
** Additional I/O ports used by the bootstrap code
*/

// keyboard controller
#define KBD_DATA        0x60
#define KBD_CMD         0x64
#define KBD_STAT        0x64

// status register bits
#define	KBD_OBSTAT      0x01
#define	KBD_IBSTAT      0x02
#define KBD_SYSFLAG     0x04
#define KBD_CMDDAT      0x08

// commands
#define KBD_P1_DISABLE  0xad
#define KBD_P1_ENABLE   0xae
#define KBD_RD_OPORT    0xd0
#define KBD_WT_OPORT    0xd1

#ifdef ASM_SRC

// segment descriptor macros for use in assembly source files
// layout:
//     .word    lower 16 bits of limit
//     .word    lower 16 bits of base
//     .byte    middle 8 bits of base
//     .byte    type byte
//     .byte    granularity byte
//     .byte    upper 8 bits of base
// we use 4K units, so we ignore the lower 12 bits of the limit
#define SEGNULL       \
	.word 0, 0, 0, 0

#define SEGMENT(base,limit,dpl,type) \
	.word (((limit) >> 12) & 0xffff); \
	.word ((base) & 0xffff) ; \
	.byte (((base) >> 16) & 0xff) ; \
	.byte (SEG_PRESENT | (dpl) | SEG_NON_SYSTEM | (type)) ; \
	.byte (SEG_GRAN_4KBYTE | SEG_DB_32BIT | (((limit) >> 28) & 0xf)) ; \
	.byte (((base) >> 24) & 0xff)

#endif  /* ASM_SRC */

#endif
