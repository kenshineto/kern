/*
** @file	uart.h
**
** @author	M. Reek
** @authors	K. Reek, Warren R. Carithers
**
** Definitions for a 16540/16550 compatible UART.  Definitions are taken
** from datasheets for the National Semiconductor INS8250, NS16450, and
** NS16550 UART chips, and the PC87309 Super I/O legacy peripheral chip.
**
** The naming convention is UAx_yyy_zzzzz.  "x" is either 4 or 5 (see below),
** "yyy" is the name of the register to which this value applies, and
** "zzzzz" is the name of the value or field.
**
** The UA4 prefix denotes 16540 compatible functions, available in both
** chips. The UA5 prefix denotes 16550-only functions (primarily the FIFOs).
**
** For many items there are two names: one short one that matches the name
** in the chip manual, and another that is more readable.
*/

#ifndef UART_H
#define	UART_H

/*********************************************************************
***************************** I/O PORTS ******************************
*********************************************************************/

/*
** Base port number assigned to the device
*/
#define	UA4_COM1_PORT	0x3f8
#define	UA4_COM2_PORT	0x2f8
#define	UA4_COM3_PORT	0x3e8
#define	UA4_COM4_PORT	0x2e8

// short name for the one we'll use
#define	UA4_PORT		UA4_COM1_PORT
#define UA5_PORT		UA4_COM1_PORT

/*
** Registers
**
** The 164x0 chips have the following registers. The (RO) and (WO)
** suffixes indicate read-only and write-only access.
**
**   Index  Register(s)
**   =====  =========================================
**     0    Receiver Data (RO), Transmitter Data (WO)
**     1    Interrupt Enable 
**     2    Interrupt ID (RO), FIFO Control (WO)
**     3    Line Control, Divisor Latch
**     4    Modem Control
**     5    Line Status
**     6    Modem Status
**     7    Scratch
**
** Registers indices are relative to the base I/O port for the
** specific UART port being used (e.g., for COM1, the port addresses
** are 0x3f8 through 0x3ff). When two registers share a port and have
** different access methods (RO vs. WO), a read from the port accesses
** the RO register and a write to the port access the WO register.
**
** The Line Control and Divisor Latch registers are accessed by writing
** a byte to the port; the high-order bit determines which register is
** accessed (0 selects Line Control, 1 selects Divisor Latch), with the
** remaining bits selecting fields within the indicated register.
*/

/*
** Receiver Data Register (read-only)
*/
#define	UA4_RXD				(UA4_PORT+0)
#	define	UA4_RX_DATA			UA4_RXD

/*
** Transmitter Data Register (write-only)
*/
#define	UA4_TXD				(UA4_PORT+0)
#	define	UA4_TX_DATA			UA4_TXD

/*
** Interrupt Enable Register
*/
#define	UA4_IER				(UA4_PORT+1)
#	define	UA4_INT_ENABLE_REG	UA4_IER

// fields
#define	UA4_IER_RX_IE		0x01	// Rcvr High-Data-Level Int Enable
#define	UA4_IER_TX_IE		0x02	// Xmitter Low-data-level Int Enable
#define	UA4_IER_LS_IE		0x04	// Line Status Int Enable
#define	UA4_IER_MS_IE		0x08	// Modem Status Int Enable

// aliases
#define	UA4_IER_RX_INT_ENABLE			UA4_IER_RX_IE
#define	UA4_IER_TX_INT_ENABLE			UA4_IER_TX_IE
#define	UA4_IER_LINE_STATUS_INT_ENABLE	UA4_IER_LS_IE
#define	UA4_IER_MODEM_STATUS_INT_ENABLE	UA4_IER_MS_IE

/*
** Interrupt Identification Register (read-only)
**
** a.k.a. Event Identification Register
*/
#define	UA4_IIR				(UA4_PORT+2)
#	define	UA4_EVENT_ID		UA4_IIR

// fields
#define	UA4_IIR_IPF			0x01	// Interrupt Pending flag

#define	UA4_IIR_IPR_MASK	0x06	// Interrupt Priority mask
#	define	UA4_IIR_IPR0_MASK	0x02	// IPR bit 0 mask
#	define	UA4_IIR_IPR1_MASK	0x04	// IPR bit 1 mask

#define	UA5_IIR_RXFT		0x08	// RX_FIFO Timeout
#define	UA5_IIR_FEN0		0x40	// FIFOs Enabled
#define	UA5_IIR_FEN1		0x80	// FIFOs Enabled

// aliases
#define	UA4_IIR_INT_PENDING		UA4_IIR_IPF
#define	UA4_IIR_INT_PRIORITY	UA4_IIR_IPR
#define	UA5_IIR_RX_FIFO_TIMEOUT	UA5_IIR_RXFT
#define	UA5_IIR_FIFO_ENABLED_0	UA5_IIR_FEN0
#define	UA5_IIR_FIFO_ENABLED_1	UA5_IIR_FEN1

// IIR interrupt priorities (four-bit values)
#define	UA4_IIR_INT_PRI_MASK	0x0f	// Mask for extracting int priority
#	define	UA4_IIR_NO_INT			0x01	// no interrupt
#	define	UA4_IIR_LINE_STATUS		0x06	// line status interrupt
#	define	UA4_IIR_RX				0x04	// Receiver High Data Level
#	define	UA5_IIR_RX_FIFO			0x0c	// Receiver FIFO timeout (16550)
#	define	UA4_IIR_TX				0x02	// Transmitter Low Data level
#	define	UA4_IIR_MODEM_STATUS	0x00	// Modem Status

// aliases
#define	UA4_IIR_NO_INT_PENDING				UA4_IIR_NO_INT
#define	UA4_IIR_LINE_STATUS_INT_PENDING		UA4_IIR_LINE_STATUS
#define	UA4_IIR_RX_INT_PENDING				UA4_IIR_RX
#define	UA5_IIR_RX_FIFO_TIMEOUT_INT_PENDING	UA5_IIR_RX_FIFO
#define	UA4_IIR_TX_INT_PENDING				UA4_IIR_TX
#define	UA4_IIR_MODEM_STATUS_INT_PENDING	UA4_IIR_MODEM_STATUS

/*
** FIFO Control Register (16550 only, write-only)
*/
#define	UA5_FCR				(UA5_PORT+2)
#	define	UA5_FIFO_CTL		UA5_FCR

#define	UA5_FCR_FIFO_RESET	0x00	// Reset the FIFO
#define	UA5_FCR_FIFO_EN		0x01	// FIFO Enable
#define	UA5_FCR_RXSR		0x02	// Receiver Soft Reset
#define	UA5_FCR_TXSR		0x04	// Transmitter Soft Reset

#define	UA5_FCR_TXFT_MASK	0x30	// TX_FIFO threshold level mask
#	define	UA5_FCR_TXFT0_MASK	0x10	// TXFT bit 0 mask
#	define	UA5_FCR_TXFT1_MASK	0x20	// TXFT bit 1 mask
#		define	UA5_FCR_TX_FIFO_1	0x00	// 1 char
#		define	UA5_FCR_TX_FIFO_3	0x10	// 3 char
#		define	UA5_FCR_TX_FIFO_9	0x20	// 9 char
#		define	UA5_FCR_TX_FIFO_13	0x30	// 13 char

#define	UA5_FCR_RXFT_MASK	0xc0	// RX_FIFO threshold level mask
#	define	UA5_FCR_RXFT0_MASK	0x40	// RXFT bit 0 mask
#	define	UA5_FCR_RXFT1_MASK	0x80	// RXFT bit 1 mask
#		define	UA5_FCR_RX_FIFO_1	0x00	// 1 char
#		define	UA5_FCR_RX_FIFO_4	0x40	// 4 char
#		define	UA5_FCR_RX_FIFO_8	0x80	// 8 char
#		define	UA5_FCR_RX_FIFO_14	0xc0	// 14 char

// aliases
#define	UA5_FCR_FIFO_ENABLED	UA5_FCR_FIFO_EN
#define	UA5_FCR_RX_SOFT_RESET	UA5_FCR_RXSR
#define	UA5_FCR_TX_SOFT_RESET	UA5_FCR_TXSR
#define	UA5_FCR_TX_FIFO_1_CHAR	UA5_FCR_TX_FIFO_1
#define	UA5_FCR_TX_FIFO_3_CHAR	UA5_FCR_TX_FIFO_3
#define	UA5_FCR_TX_FIFO_9_CHAR	UA5_FCR_TX_FIFO_9
#define	UA5_FCR_TX_FIFO_13_CHAR	UA5_FCR_TX_FIFO_13
#define	UA5_FCR_RX_FIFO_1_CHAR	UA5_FCR_RX_FIFO_1
#define	UA5_FCR_RX_FIFO_4_CHAR	UA5_FCR_RX_FIFO_4
#define	UA5_FCR_RX_FIFO_8_CHAR	UA5_FCR_RX_FIFO_8
#define	UA5_FCR_RX_FIFO_14_CHAR	UA5_FCR_RX_FIFO_14

/*
** Line Control Register (available in all banks)
**
** Selected when bit 7 of the value written to the port is a 0.
*/
#define	UA4_LCR				(UA4_PORT+3)
#	define	UA4_LINE_CTL		UA4_LCR

#define	UA4_LCR_WLS_MASK	0x03	// Word Length Select mask
#	define	UA4_LCR_WLS0_MASK	0x01	// WLS bit 0 mask
#	define	UA4_LCR_WLS1_MASK	0x02	// WLS bit 1 mask
#		define	UA4_LCR_WLS_5		0x00	// 5 bits per char
#		define	UA4_LCR_WLS_6		0x01	// 6 bits per char
#		define	UA4_LCR_WLS_7		0x02	// 7 bits per char
#		define	UA4_LCR_WLS_8		0x03	// 8 bits per char

#define	UA4_LCR_STB			0x04	// Stop Bits
#		define	UA4_LCR_1_STOP_BIT	0x00
#		define	UA4_LCR_2_STOP_BIT	0x04

#define	UA4_LCR_PEN			0x08	// Parity Enable
#define	UA4_LCR_EPS			0x10	// Even Parity Select
#define	UA4_LCR_STKP		0x20	// Sticky Parity
#		define	UA4_LCR_NO_PARITY		0x00
#		define	UA4_LCR_ODD_PARITY		UA4_LCR_PEN
#		define	UA4_LCR_EVEN_PARITY		(UA4_LCR_PEN|UA4_LCR_EPS)
#		define	UA4_LCR_PARITY_LOGIC_1	(UA4_LCR_PEN|UA4_LCR_STKP)
#		define	UA4_LCR_PARITY_LOGIC_0	(UA4_LCR_PEN|UA4_LCR_EPS|UA4_LCR_STKP)

#define	UA4_LCR_SBRK		0x40	// Set Break
#define	UA4_LCR_DLAB		0x80	// Divisor Latch select bit

// aliases
#	define	UA4_LCR_STOP_BITS		UA4_LCR_STB
#	define	UA4_LCR_PARITY_ENABLE	UA4_LCR_PEN
#	define	UA4_LCR_SET_BREAK		UA4_LCR_SBRK
#	define	UA4_LCR_BANK_SELECT_ENABLE	UA4_LCR_BKSE

/*
** Divisor Latch Registers
**     Divisor Latch Least Significant (DLL)
**     Divisor Latch Most Significant (DLM)
**
** These contain the lower and upper halves of the 16-bit divisor for
** baud rate generation.
**
** Accessing them requires sending a command to LCR with the most
** significant bit (0x80, the DLAB field) set. This "unlocks" the
** Divisor Latch registers, which are accessed at UA4_PORT+0 and
** UA4_PORT+1 (i.e., in place of the RXD/TXD and IE registers). To
** "re-lock" the Divisor Latch registers, write a command byte to
** LCR with 0 in the DLAB bit.
*/
#define	UA4_DLL				(UA4_PORT+0)	// Divisor Latch (least sig.)
#define	UA4_DLM				(UA4_PORT+1)	// Divisor Latch (most sig.)

// aliases
#define	UA4_DIVISOR_LATCH_LS	UA4_DLL
#define	UA4_DIVISOR_LATCH_MS	UA4_DLM

// Baud rate divisor high and low bytes
#define	BAUD_HIGH_BYTE(x)	(((x) >> 8) & 0xff)
#define	BAUD_LOW_BYTE(x)	((x) & 0xff)

// Baud rate divisors
#define	DL_BAUD_50			2304
#define	DL_BAUD_75			1536
#define	DL_BAUD_110			1047
#define	DL_BAUD_150			768
#define	DL_BAUD_300			384
#define	DL_BAUD_600			192
#define	DL_BAUD_1200		96
#define	DL_BAUD_1800		64
#define	DL_BAUD_2000		58
#define	DL_BAUD_2400		48
#define	DL_BAUD_3600		32
#define	DL_BAUD_4800		24
#define	DL_BAUD_7200		16
#define	DL_BAUD_9600		12
#define	DL_BAUD_14400		8
#define	DL_BAUD_19200		6
#define	DL_BAUD_28800		4
#define	DL_BAUD_38400		3
#define	DL_BAUD_57600		2
#define	DL_BAUD_115200		1

/*
** Modem Control Register
*/
#define	UA4_MCR				(UA4_PORT+4)
#	define	UA4_MODEM_CTL		UA4_MCR

#define	UA4_MCR_DTR			0x01	// Data Terminal Ready
#define	UA4_MCR_RTS			0x02	// Ready to Send
#define	UA4_MCR_RILP		0x04	// Loopback Interrupt Request
#define	UA4_MCR_ISEN		0x08	// Interrupt Signal Enable
#define	UA4_MCR_DCDLP		0x08	// DCD Loopback
#define	UA4_MCR_LOOP		0x10	// Loopback Enable

// aliases
#define	UA4_MCR_DATA_TERMINAL_READY		UA4_MCR_DTR
#define	UA4_MCR_READY_TO_SEND			UA4_MCR_RTS
#define	UA4_MCR_LOOPBACK_INT_REQ		UA4_MCR_RILP
#define	UA4_MCR_INT_SIGNAL_ENABLE		UA4_MCR_ISEN
#define	UA4_MCR_LOOPBACK_DCD			UA4_MCR_DCDLP
#define	UA4_MCR_LOOPBACK_ENABLE			UA4_MCR_LOOP

/*
** Line Status Register
*/
#define	UA4_LSR				(UA4_PORT+5)
#	define	UA4_LINE_STATUS		UA4_LSR

#define	UA4_LSR_RXDA		0x01	// Receiver Data Available
#define	UA4_LSR_OE			0x02	// Overrun Error
#define	UA4_LSR_PE			0x04	// Parity Error
#define	UA4_LSR_FE			0x08	// Framing Error
#define	UA4_LSR_BRK			0x10	// Break Event Detected
#define	UA4_LSR_TXRDY		0x20	// Transmitter Ready
#define	UA4_LSR_TXEMP		0x40	// Transmitter Empty
#define	UA4_LSR_ER_INF		0x80	// Error in RX_FIFO

// aliases
#define	UA4_LSR_RX_DATA_AVAILABLE	UA4_LSR_RXDA
#define	UA4_LSR_OVERRUN_ERROR		UA4_LSR_OE
#define	UA4_LSR_PARITY_ERROR		UA4_LSR_PE
#define	UA4_LSR_FRAMING_ERROR		UA4_LSR_FE
#define	UA4_LSR_BREAK_DETECTED		UA4_LSR_BRK
#define	UA4_LSR_TX_READY			UA4_LSR_TXRDY
#define	UA4_LSR_TX_EMPTY			UA4_LSR_TXEMP
#define	UA4_LSR_RX_FIFO_ERROR		UA4_LSR_ER_INF

/*
** Modem Status Register
*/
#define	UA4_MSR				(UA4_PORT+6)
#	define	UA4_MODEM_STATUS	UA4_MSR

#define	UA4_MSR_DCTS		0x01	// Delta Clear to Send
#define	UA4_MSR_DDSR		0x02	// Delta Data Set Ready
#define	UA4_MSR_TERI		0x04	// Trailing Edge Ring Indicate
#define	UA4_MSR_DDCD		0x08	// Delta Data Carrier Detect
#define	UA4_MSR_CTS			0x10	// Clear to Send
#define	UA4_MSR_DSR			0x20	// Data Set Ready
#define	UA4_MSR_RI			0x40	// Ring Indicate
#define	UA4_MSR_DCD			0x80	// Data Carrier Detect

// aliases
#define	UA4_MSR_DELTA_CLEAR_TO_SEND			UA4_MSR_DCTS
#define	UA4_MSR_DELTA_DATA_SET_READY		UA4_MSR_DDSR
#define	UA4_MSR_TRAILING_EDGE_RING			UA4_MSR_TERI
#define	UA4_MSR_DELTA_DATA_CARRIER_DETECT	UA4_MSR_DDCD
#define	UA4_MSR_CLEAR_TO_SEND				UA4_MSR_CTS
#define	UA4_MSR_DATA_SET_READY				UA4_MSR_DSR
#define	UA4_MSR_RING_INDICATE				UA4_MSR_RI
#define	UA4_MSR_DATA_CARRIER_DETECT			UA4_MSR_DCD

/*
** Scratch Register
**
** Not used by the UART; usable as a "scratchpad" register for
** temporary storage.
*/
#define	UA4_SCR				(UA4_PORT+7)
#	define	UA4_SCRATCH			UA4_UA5_SCR

#endif	/* uart.h */
