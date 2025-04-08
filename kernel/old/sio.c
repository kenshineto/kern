/**
** @file	sio.c
**
** @author	Warren R. Carithers
**
** @brief	SIO module
**
** For maximum compatibility from semester to semester, this code uses
** several "stand-in" type names and macros which should be defined
** in the accompanying "compat.h" header file if they're not part of
** the baseline system:
**
**      standard-sized integer types:  intN_t, uintN_t
**      other types:  PCBTYPE, QTYPE
**      scheduler functions:  SCHED, DISPATCH
**      queue functions:  QCREATE, QLENGTH, QDEQUE
**      other functions:  SLENGTH
**      sio read queue:  QNAME
**
** Our SIO scheme is very simple:
**
**  Input:  We maintain a buffer of incoming characters that haven't
**      yet been read by processes.  When a character comes in, if
**      there is no process waiting for it, it goes in the buffer;
**      otherwise, the first waiting process is awakeneda and it
**      gets the character.
**
**      When a process invokes readch(), if there is a character in
**      the input buffer, the process gets it; otherwise, it is
**      blocked until input appears
**
**      Communication with system calls is via two routines.
**      sio_readc() returns the first available character (if
**      there is one), resetting the input variables if this was
**      the last character in the buffer.  If there are no
**      characters in the buffer, sio_read() returns a -1
**      (presumably so the requesting process can be blocked).
**
**      sio_read() copies the contents of the input buffer into
**      a user-supplied buffer.  It returns the number of characters
**      copied.  If there are no characters available, return a -1.
**
**  Output: We maintain a buffer of outgoing characters that haven't
**      yet been sent to the device, and an indication of whether
**      or not we are in the middle of a transmit sequence.  When
**      an interrupt comes in, if there is another character to
**      send we copy it to the transmitter buffer; otherwise, we
**      end the transmit sequence.
**
**      Communication with user processes is via three functions.
**      sio_writec() writes a single character; sio_write()
**      writes a sized buffer full of characters; sio_puts()
**      prints a NUL-terminated string.  If we are in the middle
**      of a transmit sequence, all characters will be added
**      to the output buffer (from where they will be sent
**      automatically); otherwise, we send the first character
**      directly, add the rest of the characters (if there are
**      any) to the output buffer, and set the "sending" flag
**      to indicate that we're expecting a transmitter interrupt.
*/

#define KERNEL_SRC

// this should do all includes required for this OS
#include <compat.h>

// all other framework includes are next
#include <x86/uart.h>
#include <x86/arch.h>
#include <x86/pic.h>

#include <sio.h>
#include <lib.h>

/*
** PRIVATE DEFINITIONS
*/

#define BUF_SIZE    2048

/*
** PRIVATE GLOBALS
*/

	// input character buffer
static char inbuffer[ BUF_SIZE ];
static char *inlast;
static char *innext;
static uint32_t incount;

	// output character buffer
static char outbuffer[ BUF_SIZE ];
static char *outlast;
static char *outnext;
static uint32_t outcount;

	// output control flag
static int sending;

	// interrupt register status
static uint8_t ier;

/*
** PUBLIC GLOBAL VARIABLES
*/

// queue for read-blocked processes
#ifdef QNAME
QTYPE QNAME;
#endif

/*
** PRIVATE FUNCTIONS
*/

/**
** sio_isr(vector,ecode)
**
** Interrupt handler for the SIO module.  Handles all pending
** events (as described by the SIO controller).
**
** @param vector   The interrupt vector number for this interrupt
** @param ecode    The error code associated with this interrupt
*/
static void sio_isr( int vector, int ecode ) {
	int ch;

#if TRACING_SIO_ISR
	cio_puts( "SIO: int:" );
#endif
	//
	// Must process all pending events; loop until the IRR
	// says there's nothing else to do.
	//

	for(;;) {

		// get the "pending event" indicator
		int iir = inb( UA4_IIR ) & UA4_IIR_INT_PRI_MASK;

		// process this event
		switch( iir ) {

		case UA4_IIR_LINE_STATUS:
			// shouldn't happen, but just in case....
			cio_printf( "** SIO int, LSR = %02x\n", inb(UA4_LSR) );
			break;

		case UA4_IIR_RX:
#if TRACING_SIO_ISR
	cio_puts( " RX" );
#endif
			// get the character
			ch = inb( UA4_RXD );
			if( ch == '\r' ) {    // map CR to LF
				ch = '\n';
			}
#if TRACING_SIO_ISR
	cio_printf( " ch %02x", ch );
#endif

#ifdef QNAME
			//
			// If there is a waiting process, this must be
			// the first input character; give it to that
			// process and awaken the process.
			//

			if( !QEMPTY(QNAME) ) {
				PCBTYPE *pcb;

				QDEQUE( QNAME, pcb );
				// make sure we got a non-NULL result
				assert( pcb );

				// return char via arg #2 and count in EAX
				char *buf = (char *) ARG(pcb,2);
				*buf = ch & 0xff;
				RET(pcb) = 1;
				SCHED( pcb );

			} else {
#endif /* QNAME */

				//
				// Nobody waiting - add to the input buffer
				// if there is room, otherwise just ignore it.
				//

				if( incount < BUF_SIZE ) {
					*inlast++ = ch;
					++incount;
				}

#ifdef QNAME
			}
#endif /* QNAME */
			break;

		case UA5_IIR_RX_FIFO:
			// shouldn't happen, but just in case....
			ch = inb( UA4_RXD );
			cio_printf( "** SIO FIFO timeout, RXD = %02x\n", ch );
			break;

		case UA4_IIR_TX:
#if TRACING_SIO_ISR
	cio_puts( " TX" );
#endif
			// if there is another character, send it
			if( sending && outcount > 0 ) {
#if TRACING_SIO_ISR
	cio_printf( " ch %02x", *outnext );
#endif
				outb( UA4_TXD, *outnext );
				++outnext;
				// wrap around if necessary
				if( outnext >= (outbuffer + BUF_SIZE) ) {
					outnext = outbuffer;
				}
				--outcount;
#if TRACING_SIO_ISR
	cio_printf( " (outcount %d)", outcount );
#endif
			} else {
#if TRACING_SIO_ISR
	cio_puts( " EOS" );
#endif
				// no more data - reset the output vars
				outcount = 0;
				outlast = outnext = outbuffer;
				sending = 0;
				// disable TX interrupts
				sio_disable( SIO_TX );
			}
			break;

		case UA4_IIR_NO_INT:
#if TRACING_SIO_ISR
	cio_puts( " EOI\n" );
#endif
			// nothing to do - tell the PIC we're done
			outb( PIC1_CMD, PIC_EOI );
			return;

		case UA4_IIR_MODEM_STATUS:
			// shouldn't happen, but just in case....
			cio_printf( "** SIO int, MSR = %02x\n", inb(UA4_MSR) );
			break;

		default:
			// uh-oh....
			sprint( b256, "sio isr: IIR %02x\n", ((uint32_t) iir) & 0xff );
			PANIC( 0, b256 );
		}
	
	}

	// should never reach this point!
	assert( false );
}

/*
** PUBLIC FUNCTIONS
*/

/**
** sio_init()
**
** Initialize the UART chip.
*/
void sio_init( void ) {

#if TRACING_INIT
	cio_puts( " Sio" );
#endif

	/*
	** Initialize SIO variables.
	*/

	memclr( (void *) inbuffer, sizeof(inbuffer) );
	inlast = innext = inbuffer;
	incount = 0;

	memclr( (void *) outbuffer, sizeof(outbuffer) );
	outlast = outnext = outbuffer;
	outcount = 0;
	sending = 0;

	// queue of read-blocked processes
	QCREATE( QNAME );

	/*
	** Next, initialize the UART.
	**
	** Initialize the FIFOs
	**
	** this is a bizarre little sequence of operations
	*/

	outb( UA5_FCR, 0x20 );
	outb( UA5_FCR, UA5_FCR_FIFO_RESET );               // 0x00
	outb( UA5_FCR, UA5_FCR_FIFO_EN );                  // 0x01
	outb( UA5_FCR, UA5_FCR_FIFO_EN | UA5_FCR_RXSR );   // 0x03
	outb( UA5_FCR, UA5_FCR_FIFO_EN | UA5_FCR_RXSR | UA5_FCR_TXSR );  // 0x07

	/*
	** disable interrupts
	**
	** note that we leave them disabled; sio_enable() must be
	** called to switch them back on
	*/

	outb( UA4_IER, 0 );
	ier = 0;

	/*
	** select the divisor latch registers and set the data rate
	*/

	outb( UA4_LCR, UA4_LCR_DLAB );
	outb( UA4_DLL, BAUD_LOW_BYTE( DL_BAUD_9600 ) );
	outb( UA4_DLM, BAUD_HIGH_BYTE( DL_BAUD_9600 ) );

	/*
	** deselect the latch registers, by setting the data
	** characteristics in the LCR
	*/

	outb( UA4_LCR, UA4_LCR_WLS_8 | UA4_LCR_1_STOP_BIT | UA4_LCR_NO_PARITY );
	
	/*
	** Set the ISEN bit to enable the interrupt request signal,
	** and the DTR and RTS bits to enable two-way communication.
	*/

	outb( UA4_MCR, UA4_MCR_ISEN | UA4_MCR_DTR | UA4_MCR_RTS );

	/*
	** Install our ISR
	*/

	install_isr( VEC_COM1, sio_isr );
}

/**
** sio_enable()
**
** Enable SIO interrupts
**
** usage:    uint8_t old = sio_enable( uint8_t which )
**
** @param which   Bit mask indicating which interrupt(s) to enable
**
** @return the prior IER setting
*/
uint8_t sio_enable( uint8_t which ) {
	uint8_t old;

	// remember the current status

	old = ier;

	// figure out what to enable

	if( which & SIO_TX ) {
		ier |= UA4_IER_TX_IE;
	}

	if( which & SIO_RX ) {
		ier |= UA4_IER_RX_IE;
	}

	// if there was a change, make it

	if( old != ier ) {
		outb( UA4_IER, ier );
	}

	// return the prior settings

	return( old );
}

/**
** sio_disable()
**
** Disable SIO interrupts
**
** usage:    uint8_t old = sio_disable( uint8_t which )
**
** @param which   Bit mask indicating which interrupt(s) to disable
**
** @return the prior IER setting
*/
uint8_t sio_disable( uint8_t which ) {
	uint8_t old;

	// remember the current status

	old = ier;

	// figure out what to disable

	if( which & SIO_TX ) {
		ier &= ~UA4_IER_TX_IE;
	}

	if( which & SIO_RX ) {
		ier &= ~UA4_IER_RX_IE;
	}

	// if there was a change, make it

	if( old != ier ) {
		outb( UA4_IER, ier );
	}

	// return the prior settings

	return( old );
}

/**
** sio_inq_length()
**
** Get the input queue length
**
** usage:    int num = sio_inq_length()
**
** @return the count of characters still in the input queue
*/
int sio_inq_length( void ) {
	return( incount );
}

/**
** sio_readc()
**
** Get the next input character
**
** usage:    int ch = sio_readc()
**
** @return the next character, or -1 if no character is available
*/
int sio_readc( void ) {
	int ch;

	// assume there is no character available
	ch = -1;

	// 
	// If there is a character, return it
	//

	if( incount > 0 ) {

		// take it out of the input buffer
		ch = ((int)(*innext++)) & 0xff;
		--incount;

		// reset the buffer variables if this was the last one
		if( incount < 1 ) {
			inlast = innext = inbuffer;
		}

	}

	return( ch );

}

/**
** sio_read(buf,length)
**
** Read the entire input buffer into a user buffer of a specified size
**
** usage:    int num = sio_read( char *buffer, int length )
**
** @param buf     The destination buffer
** @param length  Length of the buffer
**
** @return the number of bytes copied, or 0 if no characters were available
*/

int sio_read( char *buf, int length ) {
	char *ptr = buf;
	int copied = 0;

	// if there are no characters, just return 0

	if( incount < 1 ) {
		return( 0 );
	}

	//
	// We have characters.  Copy as many of them into the user
	// buffer as will fit.
	//

	while( incount > 0 && copied < length ) {
		*ptr++ = *innext++ & 0xff;
		if( innext > (inbuffer + BUF_SIZE) ) {
			innext = inbuffer;
		}
		--incount;
		++copied;
	}

	// reset the input buffer if necessary

	if( incount < 1 ) {
		inlast = innext = inbuffer;
	}

	// return the copy count

	return( copied );
}


/**
** sio_writec( ch )
**
** Write a character to the serial output
**
** usage:    sio_writec( int ch )
**
** @param ch   Character to be written (in the low-order 8 bits)
*/
void sio_writec( int ch ){


	//
	// Must do LF -> CRLF mapping
	//

	if( ch == '\n' ) {
		sio_writec( '\r' );
	}

	//
	// If we're currently transmitting, just add this to the buffer
	//

	if( sending ) {
		*outlast++ = ch;
		++outcount;
		return;
	}

	//
	// Not sending - must prime the pump
	//

	sending = 1;
	outb( UA4_TXD, ch );

	// Also must enable transmitter interrupts

	sio_enable( SIO_TX );

}

/**
** sio_write( buffer, length )
**
** Write a buffer of characters to the serial output
**
** usage:    int num = sio_write( const char *buffer, int length )
**
** @param buffer   Buffer containing characters to write
** @param length   Number of characters to write
**
** @return the number of characters copied into the SIO output buffer
*/
int sio_write( const char *buffer, int length ) {
	int first = *buffer;
	const char *ptr = buffer;
	int copied = 0;

	//
	// If we are currently sending, we want to append all
	// the characters to the output buffer; else, we want
	// to append all but the first character, and then use
	// sio_writec() to send the first one out.
	//

	if( !sending ) {
		ptr += 1;
		copied++;
	}

	while( copied < length && outcount < BUF_SIZE ) {
		*outlast++ = *ptr++;
		// wrap around if necessary
		if( outlast >= (outbuffer + BUF_SIZE) ) {
			outlast = outbuffer;
		}
		++outcount;
		++copied;
	}

	//
	// We use sio_writec() to send out the first character,
	// as it will correctly set all the other necessary
	// variables for us.
	//

	if( !sending ) {
		sio_writec( first );
	}

	// Return the transfer count


	return( copied );

}

/**
** sio_puts( buf )
**
** Write a NUL-terminated buffer of characters to the serial output
**
** usage:    int num = sio_puts( const char *buffer )
**
** @param buffer  The buffer containing a NUL-terminated string
**
** @return the count of bytes transferred
*/
int sio_puts( const char *buffer ) {
	int n;  // must be outside the loop so we can return it

	n = SLENGTH( buffer );
	sio_write( buffer, n );

	return( n );
}

/**
** sio_dump( full )
**
** dump the contents of the SIO buffers to the console
**
** usage:    sio_dump(true) or sio_dump(false)
**
** @param full   Boolean indicating whether or not a "full" dump
**               is being requested (which includes the contents
**               of the queues)
*/

void sio_dump( bool_t full ) {
	int n;
	char *ptr;

	// dump basic info into the status region

	cio_printf_at( 48, 0,
		"SIO: IER %02x (%c%c%c) in %d ot %d",
			((uint32_t)ier) & 0xff, sending ? '*' : '.',
			(ier & UA4_IER_TX_IE) ? 'T' : 't',
			(ier & UA4_IER_RX_IE) ? 'R' : 'r',
			incount, outcount );

	// if we're not doing a full dump, stop now

	if( !full ) {
		return;
	}

	// also want the queue contents, but we'll
	// dump them into the scrolling region

	if( incount ) {
		cio_puts( "SIO input queue: \"" );
		ptr = innext; 
		for( n = 0; n < incount; ++n ) {
			put_char_or_code( *ptr++ );
		}
		cio_puts( "\"\n" );
	}

	if( outcount ) {
		cio_puts( "SIO output queue: \"" );
		cio_puts( " ot: \"" );
		ptr = outnext; 
		for( n = 0; n < outcount; ++n )  {
			put_char_or_code( *ptr++ );
		}
		cio_puts( "\"\n" );
	}
}
