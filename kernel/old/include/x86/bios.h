/*
** @file	bios.h
**
** @author	Warren R. Carithers
**
** BIOS-related declarations
*/

#ifndef BIOS_H_
#define BIOS_H_

/*
** BIOS-related memory addresses
*/

#define BIOS_BDA       0x0400

/*
** Selected BIOS interrupt numbers
*/

#define BIOS_TIMER     0x08
#define BIOS_KBD       0x09
#define BIOS_VIDEO     0x10
#define BIOS_EQUIP     0x11
#define BIOS_MSIZ      0x12
#define BIOS_DISK      0x13
#define BIOS_SERIAL    0x14
#define BIOS_MISC      0x15
#define BIOS_KBDSVC    0x16
#define BIOS_PRTSVC    0x17
#define BIOS_BOOT      0x19
#define BIOS_RTCPCI    0x1a

// BIOS video commands (AH)
#define BV_W_ADV     0x0e

// BIOS disk commands (AH)
#define BD_RESET     0x00
#define BD_CHECK     0x01
#define BD_RDSECT    0x02
#define BD_WRSECT    0x03
#define BD_PARAMS    0x08

// BIOS disk commands with parameters (AX)
#define BD_READ(n)   ((BD_RDSECT << 8) | (n))
#define BD_READ1     0x0201

// CMOS ports (used for masking NMIs)
#define	CMOS_ADDR    0x70
#define	CMOS_DATA    0x71

// important related commands
#define NMI_ENABLE   0x00
#define NMI_DISABLE  0x80

/*
** Physical Memory Map Table (0000:2D00 - 0000:7c00)
**
** Primarily used with the BIOS_MISC interrupt
*/
#define MMAP_SEG       0x02D0
#define MMAP_DISP      0x0000
#define MMAP_ADDR      ((MMAP_SEG << 4) + MMAP_DISP)
#define MMAP_SECTORS   0x0a

#define MMAP_ENT       24            /* bytes per entry */
#define MMAP_MAX_ENTS  (BOOT_ADDR - MMAP_ADDR - 4) / 24

#define MMAP_CODE      0xE820        /* int 0x15 code */
#define MMAP_MAGIC_NUM 0x534D4150    /* for 0xE820 interrupt */

#endif
