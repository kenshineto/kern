/**
** @file	sio.h
**
** @author	Warren R. Carithers
**
** @brief	SIO definitions
*/

#ifndef SIO_H_
#define SIO_H_

// compatibility definitions
#include <compat.h>

/*
** General (C and/or assembly) definitions
*/

// sio interrupt settings

#define SIO_TX      0x01
#define SIO_RX      0x02
#define SIO_BOTH    (SIO_TX | SIO_RX)

#ifndef ASM_SRC

/*
** Start of C-only definitions
*/

#include <common.h>

#include <procs.h>

/*
** PUBLIC GLOBAL VARIABLES
*/

// queue for read-blocked processes
extern QTYPE QNAME;

/*
** PUBLIC FUNCTIONS
*/

/**
** sio_init()
**
** Initialize the UART chip.
*/
void sio_init( void );

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
uint8_t sio_enable( uint8_t which );

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
uint8_t sio_disable( uint8_t which );

/**
** sio_inq_length()
**
** Get the input queue length
**
** usage:   int num = sio_inq_length()
**
** @return the count of characters still in the input queue
*/
int sio_inq_length( void );

/**
** sio_readc()
**
** Get the next input character
**
** usage:   int ch = sio_readc()
**
** @return the next character, or -1 if no character is available
*/
int sio_readc( void );

/**
** sio_read()
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
int sio_read( char *buffer, int length );

/**
** sio_writec( ch )
**
** Write a character to the serial output
**
** usage:   sio_writec( int ch )
**
** @param ch   Character to be written (in the low-order 8 bits)
*/
void sio_writec( int ch );

/**
** sio_write( ch )
**
** Write a buffer of characters to the serial output
**
** usage:   int num = sio_write( const char *buffer, int length )
**
** @param buffer   Buffer containing characters to write
** @param length   Number of characters to write
**
** @return the number of characters copied into the SIO output buffer
*/
int sio_write( const char *buffer, int length );

/**
** sio_puts( buf )
**
** Write a NUL-terminated buffer of characters to the serial output
**
** usage:   n = sio_puts( const char *buffer );
**
** @param buffer  The buffer containing a NUL-terminated string
**
** @return the count of bytes transferred
*/
int sio_puts( const char *buffer );

/**
** sio_dump( full )
**
** Dump the contents of the SIO buffers to the console
**
** usage:    sio_dump(true) or sio_dump(false)
**
** @param full   Boolean indicating whether or not a "full" dump
**               is being requested (which includes the contents
**               of the queues)
*/
void sio_dump( bool_t full );

#endif	/* !ASM_SRC */

#endif
