/**
** SCCS ID: @(#)support.h	2.3        1/22/25
**
** @file	support.h
**
** @author	K. Reek
** @author	Warren R. Carithers
**
** Declarations for functions provided in support.c, and
** some hardware characteristics needed in the initialization.
**
*/

#ifndef SUPPORT_H
#define SUPPORT_H

/*
** Delay values
**
** Notes:  The parameter to the delay() function is ambiguous; it
** purports to indicate a delay length, but that isn't really tied
** to any real-world time measurement.
**
** On the original systems we used (dual 500MHz Intel P3 CPUs), each
** "unit" was approximately one tenth of a second, so delay(10) would
** delay for about one second.
**
** On the current machines (Intel Core i5-7500), delay(100) is about
** 2.5 seconds, so each "unit" is roughly 0.025 seconds.
**
** Ultimately, just remember that THESE VALUES ARE APPROXIMATE AT BEST.
*/
#define DELAY_1_SEC         40
#define DELAY_1_25_SEC      50
#define DELAY_2_SEC         80
#define DELAY_2_5_SEC      100
#define DELAY_3_SEC        120
#define DELAY_5_SEC        200
#define DELAY_7_SEC        280
#define DELAY_10_SEC       400

#ifndef ASM_SRC
/**
** panic
**
** Called when we find an unrecoverable error, this routine disables
** interrupts, prints a description of the error and then goes into a
** hard loop to prevent any further processing.
**
** @param reason  NUL-terminated message to be printed.
*/
void panic( char *reason );

/**
** init_interrupts
**
** (Re)initilizes the interrupt system. This includes initializing the
** IDT and the PIC. It is up to the user to enable processor interrupts
** when they're ready.
*/
void init_interrupts( void );

/*
** install_isr
**
** Installs a second-level handler for a specific interrupt. Returns the
** previously-installed handler for reinstallation (if desired).
**
** @param vector    the interrupt vector number
** @param handler	the second-stage ISR function to be called by the stub
**
** @return a pointer to the previously-registered ISR
*/
void (*install_isr( int vector,
		void ( *handler )(int,int) ) )( int, int );

/*
** Name:    delay
**
** See the comment above about the relative accuracy of the 'length'
** parameter.
*/
void delay( int length );

#endif  /* !ASM_SRC */

#endif
