/*
** @file    arch.h
**
** @author  Warren R. Carithers
** @author	K. Reek
**
** Definitions of constants and macros for use
** with the x86 architecture and registers.
**
*/

#ifndef X86ARCH_H_
#define X86ARCH_H_

/*
** Video stuff
*/
#define VID_BASE_ADDR   0xB8000

/*
** Memory management
*/
#define SEG_PRESENT     0x80
#define SEG_PL_0        0x00
#define SEG_PL_1        0x20
#define SEG_PL_2        0x40
#define SEG_PL_3        0x50
#define SEG_SYSTEM      0x00
#define SEG_NON_SYSTEM  0x10
#define SEG_32BIT       0x04
#define DESC_IGATE      0x06

/*
** Exceptions
*/
#define N_EXCEPTIONS    256

/*
** Bit definitions in registers
**
** See IA-32 Intel Architecture SW Dev. Manual, Volume 3: System
** Programming Guide, page 2-8.
*/

/*
** EFLAGS
*/
#define EFL_RSVD 0xffc00000  /* reserved */
#define EFL_MB0  0x00008020  /* must be zero */
#define EFL_MB1  0x00000002  /* must be 1 */

#define EFL_ID   0x00200000
#define EFL_VIP  0x00100000
#define EFL_VIF  0x00080000
#define EFL_AC   0x00040000
#define EFL_VM   0x00020000
#define EFL_RF   0x00010000
#define EFL_NT   0x00004000
#define EFL_IOPL 0x00003000
#define EFL_OF   0x00000800
#define EFL_DF   0x00000400
#define EFL_IF   0x00000200
#define EFL_TF   0x00000100
#define EFL_SF   0x00000080
#define EFL_ZF   0x00000040
#define EFL_AF   0x00000010
#define EFL_PF   0x00000004
#define EFL_CF   0x00000001

/*
** CR0, CR1, CR2, CR3, CR4
**
** IA-32 V3, page 2-12.
*/
#define CR0_RSVD        0x1ffaffc0
#define CR0_PG          0x80000000
#define CR0_CD          0x40000000
#define CR0_NW          0x20000000
#define CR0_AM          0x00040000
#define CR0_WP          0x00010000
#define CR0_NE          0x00000020
#define CR0_ET          0x00000010
#define CR0_TS          0x00000008
#define CR0_EM          0x00000004
#define CR0_MP          0x00000002
#define CR0_PE          0x00000001

#define CR1_RSVD        0xffffffff

#define CR2_RSVD        0x00000000
#define CR2_PF_LIN_ADDR 0xffffffff

#define CR3_RSVD        0x00000fe7
#define CR3_PD_BASE     0xfffff000
#define CR3_PCD         0x00000010
#define CR3_PWT         0x00000008

#define CR4_RSVD        0xfd001000
#define CR4_UINT        0x02000000
#define CR4_PKS         0x01000000
#define CR4_CET         0x00800000
#define CR4_PKE         0x00400000
#define CR4_SMAP        0x00200000
#define CR4_SMEP        0x00100000
#define CR4_KL          0x00080000
#define CR4_OSXS        0x00040000
#define CR4_PCID        0x00020000
#define CR4_FSGS        0x00010000
#define CR4_SMXE        0x00004000
#define CR4_VMXE        0x00002000
#define CR4_LA57        0x00001000
#define CR4_UMIP        0x00000800
#define CR4_OSXMMEXCPT  0x00000400
#define CR4_OSFXSR      0x00000200
#define CR4_PCE         0x00000100
#define CR4_PGE         0x00000080
#define CR4_MCE         0x00000040
#define CR4_PAE         0x00000020
#define CR4_PSE         0x00000010
#define CR4_DE          0x00000008
#define CR4_TSD         0x00000004
#define CR4_PVI         0x00000002
#define CR4_VME         0x00000001

/*
** PMode segment selector field masks
**
** IA-32 V3, page 3-8.
*/
#define SEG_SEL_IX_MASK     0xfff8
#define SEG_SEL_TI_MASK     0x0004
#define SEG_SEL_RPL_MASK    0x0003

/*
** Segment descriptor bytes
**
** IA-32 V3, page 3-10.
**
** Bytes:
**    0, 1: segment limit 15:0
**    2, 3: base address 15:0
**    4:    base address 23:16
**    7:    base address 31:24
*/

/*
** Byte 5:    access control bits
**    7:    present
**    6-5:  DPL
**    4:    system/user
**    3-0:  type
*/
#define SEG_ACCESS_P_MASK       0x80
#   define SEG_PRESENT          0x80
#   define SEG_NOT_PRESENT      0x00

#define SEG_ACCESS_DPL_MASK     0x60
#   define SEG_DPL_0            0x00
#   define SEG_DPL_1            0x20
#   define SEG_DPL_2            0x40
#   define SEG_DPL_3            0x60

#define SEG_ACCESS_S_MASK       0x10
#   define SEG_SYSTEM           0x00
#   define SEG_NON_SYSTEM       0x10

#define SEG_TYPE_MASK           0x0f
#   define SEG_DATA_A_BIT       0x1
#   define SEG_DATA_W_BIT       0x2
#   define SEG_DATA_E_BIT       0x4
#   define SEG_CODE_A_BIT       0x1
#   define SEG_CODE_R_BIT       0x2
#   define SEG_CODE_C_BIT       0x4
#       define SEG_DATA_RO      0x0
#       define SEG_DATA_ROA     0x1
#       define SEG_DATA_RW      0x2
#       define SEG_DATA_RWA     0x3
#       define SEG_DATA_RO_XD   0x4
#       define SEG_DATA_RO_XDA  0x5
#       define SEG_DATA_RW_XW   0x6
#       define SEG_DATA_RW_XWA  0x7
#       define SEG_CODE_XO      0x8
#       define SEG_CODE_XOA     0x9
#       define SEG_CODE_XR      0xa
#       define SEG_CODE_XRA     0xb
#       define SEG_CODE_XO_C    0xc
#       define SEG_CODE_XO_CA   0xd
#       define SEG_CODE_XR_C    0xe
#       define SEG_CODE_XR_CA   0xf

/*
** Byte 6:    sizes
**    7:    granularity
**    6:    d/b
**    5:    long mode
**    4:    available!
**    3-0:  upper 4 bits of limit
**    7:    base address 31:24
*/
#define SEG_SIZE_G_MASK         0x80
#   define SEG_GRAN_BYTE        0x00
#   define SEG_GRAN_4KBYTE      0x80

#define SEG_SIZE_D_B_MASK       0x40
#   define SEG_DB_16BIT         0x00
#   define SEG_DB_32BIT         0x40

#define SEG_SIZE_L_MASK         0x20
#   define SEG_L_64BIT          0x20
#   define SEG_L_32BIT          0x00

#define SEG_SIZE_AVL_MASK       0x10

#define SEG_SIZE_LIM_19_16_MASK 0x0f


/*
** System-segment and gate-descriptor types
**
** IA-32 V3, page 3-15.
*/
	// type 0: reserved
#define SEG_SYS_16BIT_TSS_AVAIL 0x1
#define SEG_SYS_LDT             0x2
#define SEG_SYS_16BIT_TSS_BUSY  0x3
#define SEG_SYS_16BIT_CALL_GATE 0x4
#define SEG_SYS_TASK_GATE       0x5
#define SEG_SYS_16BIT_INT_GATE  0x6
#define SEG_SYS_16BIT_TRAP_GATE 0x7
	// type 8: reserved
#define SEG_SYS_32BIT_TSS_AVAIL 0x9
	// type A: reserved
#define SEG_SYS_32BIT_TSS_BUSY  0xb
#define SEG_SYS_32BIT_CALL_GATE 0xc
	// type D: reserved
#define SEG_SYS_32BIT_INT_GATE  0xe
#define SEG_SYS_32BIT_TRAP_GATE 0xf

/*
** IDT Descriptors
** 
** IA-32 V3, page 5-13.
**
** All have a segment selector in bytes 2 and 3; Task Gate descriptors
** have bytes 0, 1, 4, 6, and 7 reserved; others have bytes 0, 1, 6,
** and 7 devoted to the 16 bits of the Offset, with the low nybble of
** byte 4 reserved.
*/
#define IDT_PRESENT             0x8000
#define IDT_DPL_MASK            0x6000
#   define IDT_DPL_0            0x0000
#   define IDT_DPL_1            0x2000
#   define IDT_DPL_2            0x4000
#   define IDT_DPL_3            0x6000
#define IDT_GATE_TYPE           0x0f00
#   define IDT_TASK_GATE        0x0500
#   define IDT_INT16_GATE       0x0600
#   define IDT_INT32_GATE       0x0e00
#   define IDT_TRAP16_GATE      0x0700
#   define IDT_TRAP32_GATE      0x0f00

/*
** Interrupt vectors
*/

// predefined by the architecture
#define VEC_DIVIDE_ERROR            0x00
#define VEC_DEBUG_EXCEPTION         0x01
#define VEC_NMI_INTERRUPT           0x02
#define VEC_BREAKPOINT              0x03
#define VEC_OVERFLOW                0x04
#define VEC_BOUND_RANGE_EXCEEDED    0x05
#define VEC_INVALID_OPCODE          0x06
#define VEC_DEVICE_NOT_AVAILABLE    0x07
#define VEC_DOUBLE_FAULT            0x08
#define VEC_COPROCESSOR_OVERRUN     0x09
#define VEC_INVALID_TSS             0x0a
#define VEC_SEGMENT_NOT_PRESENT     0x0b
#define VEC_STACK_FAULT             0x0c
#define VEC_GENERAL_PROTECTION      0x0d
#define VEC_PAGE_FAULT              0x0e
// 0x0f is reserved - unused
#define VEC_FPU_ERROR               0x10
#define VEC_ALIGNMENT_CHECK         0x11
#define VEC_MACHINE_CHECK           0x12
#define VEC_SIMD_FP_EXCEPTION       0x13
#define VEC_VIRT_EXCEPTION          0x14
#define VEC_CTRL_PROT_EXCEPTION     0x15
// 0x16 through 0x1f are reserved

// 0x20 through 0xff are user-defined, non-reserved

// IRQ0 through IRQ15 will use vectors 0x20 through 0x2f
#define VEC_TIMER                   0x20
#define VEC_KBD                     0x21
#define VEC_COM2                    0x23
#define VEC_COM1                    0x24
#define VEC_PARALLEL                0x25
#define VEC_FLOPPY                  0x26
#define VEC_MYSTERY                 0x27
#define VEC_MOUSE                   0x2c

#endif
