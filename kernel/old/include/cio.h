/**
** SCCS ID:	@(#)cio.h	2.7	1/22/25
**
** @file	cio.h
**
** @author	Warren R. Carithers
** @authors	K. Reek, Jon Coles
**
** Based on:    c_io.c 1.13 (Ken Reek, Jon Coles, Warren R. Carithers)
**
** Declarations and descriptions of console I/O routines
**
**  These routines provide a rudimentary capability for printing to
**  the screen and reading from the keyboard.  
**
** Screen output:
**  There are two families of functions.  The first provides a window
**  that behaves in the usual manner: writes extending beyond the right
**  edge of the window wrap around to the next line, the top line
**  scrolls off the window to make room for new lines at the bottom.
**  However, you may choose what part of the screen contains this
**  scrolling window.  This allows you to print some text at fixed
**  locations on the screen while the rest of the screen scrolls.
**
**  The second family allows for printing at fixed locations on the
**  screen.  No scrolling or line wrapping are done for these functions.
**  It is not intended that these functions be used to write in the
**  scrolling area of the screen.
**
**  In both sets of functions, the (x,y) coordinates are interpreted
**  as (column,row), with the upper left corner of the screen being
**  (0,0) and the lower right corner being (79,24).
**
**  The printf provided in both sets of functions has the same
**  conversion capabilities.  Format codes are of the form:
**
**      %-0WC
**
**  where "-", "0", and "W" are all optional:
**    "-" is the left-adjust flag (default is right-adjust)
**    "0" is the zero-fill flag (default is space-fill)
**    "W" is a number specifying the minimum field width (default: 1 )
**  and "C" is the conversion type, which must be one of these:
**    "c" print a single character
**    "s" print a null-terminated string
**    "d" print an integer as a decimal value
**    "x" print an integer as a hexadecimal value
**    "o" print an integer as a octal value
**
** Keyboard input:
**  Two functions are provided: getting a single character and getting
**  a newline-terminated line.  A third function returns a count of
**  the number of characters available for immediate reading. 
**  No conversions are provided (yet).
*/

#ifndef CIO_H_
#define CIO_H_

#ifndef ASM_SRC

// EOT indicator (control-D)
#define EOT '\04'

/*****************************************************************************
**
** INITIALIZATION ROUTINES
**
**  Initializes the I/O system.
*/

/**
** cio_init
**
** Initializes the I/O routines.  Must be called before
** any CIO functions can be used.
**
** @param notify  pointer to an input notification function, or NULL
*/
void cio_init( void (*notify)(int) );

/*****************************************************************************
**
** SCROLLING OUTPUT ROUTINES
**
**  Each operation begins at the current cursor position and advances
**  it.  If a newline is output, the reminder of that line is cleared.
**  Output extending past the end of the line is wrapped.  If the
**  cursor is moved below the scrolling region's bottom edge, scrolling
**  is delayed until the next output is produced.
*/

/**
** cio_setscroll
**
** This sets the scrolling region to be the area defined by the arguments.
** The remainder of the screen does not scroll and may be used to display
** data you do not want to move.  By default, the scrolling region is the
** entire screen. X and Y coordinates begin at 0 in the upper left corner
** of the screen.
**
** @param min_x,min_y    upper-left corner of the region
** @param max_x,max_y    lower-right corner of the region
*/
void cio_setscroll( unsigned int min_x, unsigned int min_y,
		unsigned int max_x, unsigned int max_y );

/**
** cio_moveto
**
** Moves the cursor to the specified position. (0,0) indicates
** the upper left corner of the scrolling region.  Subsequent
** output will begin at the cursor position.
**
** @param x,y   desired coordinate position
*/
void cio_moveto( unsigned int x, unsigned int y );

/**
** cio_putchar
**
** Prints a single character.
**
** @param c   the character to be printed
*/
void cio_putchar( unsigned int c );

/**
** cio_puts
**
** Prints the characters in the string up to but not including
** the terminating null byte.
**
** @param str   pointer to a NUL-terminated string to be printed
*/
void cio_puts( const char *str );

/**
** cio_write
**
** Prints "length" characters from the buffer.
**
** @param buf     the buffer of characters
** @param length  the number of characters to print
*/
void cio_write( const char *buf, int length );

/**
** cio_printf
**
** Limited form of printf (see the beginning of this file for
** a list of what is implemented).
**
** @param fmt   a printf-style format control string
** @param ...   optional additional values to be printed
*/
void cio_printf( char *fmt, ... );

/**
** cio_scroll
**
** Scroll the scrolling region up by the given number of lines.
** The output routines scroll automatically so normally you
** do not need to call this routine yourself.
**
** @param lines   the number of lines to scroll
*/
void cio_scroll( unsigned int lines );

/**
** cio_clearscroll
**
** Clears the entire scrolling region to blank spaces, and
** moves the cursor to (0,0).
*/
void cio_clearscroll( void );

/*****************************************************************************
**
** NON-SCROLLING OUTPUT ROUTINES
**
**  Coordinates are relative to the entire screen: (0,0) is the upper
**  left corner.  There is no line wrap or scrolling.
*/

/**
** cio_putchar_at
**
** Prints the given character.  If a newline is printed,
** the rest of the line is cleared.  If this happens to the
** left of the scrolling region, the clearing stops when the
** region is reached.  If this happens inside the scrolling
** region, the clearing stops when the edge of the region
** is reached.
**
** @param x,y   desired coordinate position
** @param c   the character to be printed
*/
void cio_putchar_at( unsigned int x, unsigned int y, unsigned int c );

/**
** cio_puts_at
**
** Prints the given string.  cio_putchar_at is used to print
** the individual characters; see that description for details.
**
** @param x,y   desired coordinate position
** @param str   pointer to a NUL-terminated string to be printed
*/
void cio_puts_at( unsigned int x, unsigned int y, const char *str );

/**
** cio_printf_at
**
** Limited form of printf (see the beginning of this file for
** a list of what is implemented).
**
** @param x,y   desired coordinate position
** @param fmt   a printf-style format control string
** @param ...   optional additional values to be printed
*/
void cio_printf_at( unsigned int x, unsigned int y, char *fmt, ... );

/**
** cio_clearscreen
**
** This function clears the entire screen, including the scrolling region.
*/
void cio_clearscreen( void );

/*****************************************************************************
**
** INPUT ROUTINES
**
**  When interrupts are enabled, a keyboard ISR collects keystrokes
**  and saves them until the program calls for them.  If the input
**  queue fills, additional characters are silently discarded.
**  When interrupts are not enabled, keystrokes made when no input
**  routines have been **   called are lost.  This can cause errors in
**  the input translation because the states of the Shift and Ctrl keys
**  may not be tracked accurately.  If interrupts are disabled, the user
**  is advised to refrain from typing anything except when the program is
**  waiting for input.
*/

/**
** cio_getchar
**
** If the character is not immediately available, the function
** waits until the character arrives.
**
** @return  the next character from the keyboard input buffer
*/
int cio_getchar( void );

/**
** cio_gets
**
** This function reads a newline-terminated line from the
** keyboard.  cio_getchar is used to obtain the characters;
** see that description for more details.  The function
** returns when:
**          a newline is entered (this is stored in the buffer)
**          ctrl-D is entered (not stored in the buffer)
**          the buffer becomes full.
** The buffer is null-terminated in all cases.
**
** @param buffer  destination buffer for the input character sequence
** @param size    the buffer length
**
** @return count of the number of characters read into the buffer
*/
int cio_gets( char *buffer, unsigned int size );

/**
** cio_input_queue
**
** This function lets the program determine whether there is input
** available.  This indicates whether or not a call to cio_getchar()
** would block.
**
** @return number of characters in the input queue
*/
int cio_input_queue( void );
#endif  /* !ASM_SRC */

#endif
