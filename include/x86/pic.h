/**
** @file	pic.h
**
** @author	Warren R. Carithers
** @author	K. Reek
**
** Definitions of constants and macros for the Intel 8259 Programmable
** Interrupt Controller.
**
*/

#ifndef X86PIC_H_
#define X86PIC_H_

/*
** Our expected configuration is two PICs, with the secondary connected
** through the IRQ2 pin of the primary.
*/

/*
** Port addresses for the command port and interrupt mask register port
** for both the primary and secondary PICs.
*/
#define PIC1_CMD		0x20			// primary command
#define PIC1_DATA		(PIC1_CMD + 1)	// primary data / int mask register
#define PIC2_CMD		0xA0			// secondary command
#define PIC2_DATA		(PIC2_CMD + 1)	// secondary data / int mask register

/*
** Initialization Command Word (ICW) definitions
**
** Initialization sequence:
**    ICW1 Init command is sent to each command port.
**    ICW2 vector commands are sent to the data ports.
**    If "cascade mode" was selected, send ICW3 commands to the data ports.
**    If "need ICW4" was selected, send ICW4 commands to the data ports.
**
** Following that sequence, the PIC is ready to accept interrupts;
** it will also accept Output Command Words (OCWs) to the data ports.
**
** PIC1_* defines are intended for the primary PIC
** PIC2_* defines are intended for the secondary PIC
** PIC_* defines are sent to both PICs
*/

/*
** ICW1: initialization, send to command port
*/
#define PIC_CW1_INIT		0x10	// start initialization sequence
#define PIC_CW1_NEED4		0x01	// ICW4 will also be set
#define PIC_CW1_SINGLE		0x02	// select single (vs. cascade) mode
#define PIC_CW1_INTVAL		0x04	// set call interval to 4 (vs. 8)
#define PIC_CW1_LEVEL		0x08	// use level-triggered mode (vs. edge)

/*
** ICW2: interrupt vector base offsets, send to data port
*/
#define	PIC1_CW2_VECBASE	0x20	// IRQ0 int vector number
#define	PIC2_CW2_VECBASE	0x28	// IRQ8 int vector number

/*
** ICW3: secondary::primary attachment, send to data port
*/
#define PIC1_CW3_SEC_IRQ2	0x04	// bit mask: secondary is on pin 2
#define PIC2_CW3_SEC_ID		0x02	// integer: secondary id

/*
** ICW4: operating mode, send to data port
*/
#define PIC_CW4_PM86		0x01	// 8086 mode (vs. 8080/8085)
#define PIC_CW4_AUTOEOI		0x02	// do auto eoi's
#define	PIC_CW4_UNBUF		0x00	// unbuffered mode
#define PIC_CW4_SEC_BUF		0x08	// put secondary in buffered mode
#define PIC_CW4_PRI_BUF		0x0C	// put primary in buffered mode
#define PIC_CW4_SFNMODE		0x10	// "special fully nested" mode

/*
** Operation Control Words (OCWs)
**
** After the init sequence, can send these
*/

/*
** OCW1: interrupt mask; send to data port
*/
#define	PIC_MASK_NONE		0x00	// allow all interrupts
#define	PIC_MASK_NO_IRQ0	0x01	// prevent IRQ0 interrupts
#define	PIC_MASK_NO_IRQ1	0x02	// prevent IRQ1 interrupts
#define	PIC_MASK_NO_IRQ2	0x04	// prevent IRQ2 interrupts
#define	PIC_MASK_NO_IRQ3	0x08	// prevent IRQ3 interrupts
#define	PIC_MASK_NO_IRQ4	0x10	// prevent IRQ4 interrupts
#define	PIC_MASK_NO_IRQ5	0x20	// prevent IRQ5 interrupts
#define	PIC_MASK_NO_IRQ6	0x40	// prevent IRQ6 interrupts
#define	PIC_MASK_NO_IRQ7	0x80	// prevent IRQ7 interrupts
#define	PIC_MASK_ALL		0xff	// prevent all interrupts

/*
** OCW2: EOI control, interrupt level; send to command port
*/
#define	PIC_LVL_0			0x00	// act on IRQ level 0
#define	PIC_LVL_1			0x01	// act on IRQ level 1
#define	PIC_LVL_2			0x02	// act on IRQ level 2
#define	PIC_LVL_3			0x03	// act on IRQ level 3
#define	PIC_LVL_4			0x04	// act on IRQ level 4
#define	PIC_LVL_5			0x05	// act on IRQ level 5
#define	PIC_LVL_6			0x06	// act on IRQ level 6
#define	PIC_LVL_7			0x07	// act on IRQ level 7

#define	PIC_EOI_NON_SPEC	0x20	// non-specific EOI command
#	define PIC_EOI			PIC_EOI_NON_SPEC

#define	PIC_EOI_SPEC		0x60	// specific EOI command
#	define PIC_SEOI			PIC_EOI_SPEC
#	define PIC_SEOI_LVL0	(PIC_EOI_SPEC | PIC_LVL_0)
#	define PIC_SEOI_LVL1	(PIC_EOI_SPEC | PIC_LVL_1)
#	define PIC_SEOI_LVL2	(PIC_EOI_SPEC | PIC_LVL_2)
#	define PIC_SEOI_LVL3	(PIC_EOI_SPEC | PIC_LVL_3)
#	define PIC_SEOI_LVL4	(PIC_EOI_SPEC | PIC_LVL_4)
#	define PIC_SEOI_LVL5	(PIC_EOI_SPEC | PIC_LVL_5)
#	define PIC_SEOI_LVL6	(PIC_EOI_SPEC | PIC_LVL_6)
#	define PIC_SEOI_LVL7	(PIC_EOI_SPEC | PIC_LVL_7)

#define	PIC_EOI_ROT_NONSP		0xa0	// rotate on non-spec EOI cmd
#define	PIC_EOI_SET_ROT_AUTO	0x80	// set "rotate in auto EOI mode"
#define	PIC_EOI_CLR_ROT_AUTO	0x00	// clear "rotate in auto EOI mode"
#define	PIC_EOI_ROT_SPEC		0xe0	// rotate on spec EOI cmd (+ level)
#define	PIC_EOI_SET_PRIO		0xc0	// set priority (+ level)
#define	PIC_EOI_NOP				0x40	// no operation

/*
** OCW3: read requests, special mask mode; send to command port 
*/
#define PIC_READIRR			0x0a	// read the IR register
#define PIC_READISR			0x0b	// read the IS register
#define	PIC_POLL			0x0c	// poll
#define	PIC_MASK_RESET		0x48	// reset special mask mode
#define	PIC_MASK_SET		0x68	// set special mask mode

#endif
