/*
** SCCS ID:	@(#)support.c	2.6	1/22/25
**
** @file	support.c
**
** @author  4003-506 class of 20003
** @authors K. Reek, Warren R. Carithers
**
** Miscellaneous system initialization functions, interrupt
** support routines, and data structures.
*/

#include <common.h>

#include <support.h>
#include <cio.h>
#include <x86/arch.h>
#include <x86/pic.h>
#include <x86/ops.h>
#include <bootstrap.h>
#include <syscalls.h>

/*
** Global variables and local data types.
*/

/*
** This is the table that contains pointers to the C-language ISR for
** each interrupt.  These functions are called from the isr stub based
** on the interrupt number.
*/
void ( *isr_table[ 256 ] )( int vector, int code );

/*
** Format of an IDT entry.
*/
typedef struct  {
	short   offset_15_0;
	short   segment_selector;
	short   flags;
	short   offset_31_16;
} IDT_Gate;

/*
** LOCAL ROUTINES - not intended to be used outside this module.
*/

/**
** unexpected_handler
**
** This routine catches interrupts that we do not expect to ever occur.
** It handles them by (optionally) reporting them and then calling panic().
**
** @param vector   vector number for the interrupt that occurred
** @param code     error code, or a dummy value
**
** Does not return.
*/
#ifdef RPT_INT_UNEXP
/* add any header includes you need here */
#endif
static void unexpected_handler( int vector, int code ) {
#ifdef RPT_INT_UNEXP
	cio_printf( "\n** UNEXPECTED vector %d code %d\n", vector, code );
#endif
	panic( "Unexpected interrupt" );
}

/**
** default_handler
**
** Default handler for interrupts we expect may occur but are not
** handling (yet).  We just reset the PIC and return.
**
** @param vector   vector number for the interrupt that occurred
** @param code     error code, or a dummy value
*/
static void default_handler( int vector, int code ) {
#ifdef RPT_INT_UNEXP
	cio_printf( "\n** vector %d code %d\n", vector, code );
#endif
	if( vector >= 0x20 && vector < 0x30 ) {
		if( vector > 0x27 ) {
			// must also ACK the secondary PIC
			outb( PIC2_CMD, PIC_EOI );
		}
		outb( PIC1_CMD, PIC_EOI );
	} else {
		/*
		** All the "expected" interrupts will be handled by the
		** code above.  If we get down here, the isr table may
		** have been corrupted.  Print a message and don't return.
		*/
		panic( "Unexpected \"expected\" interrupt!" );
	}
}

/**
** mystery_handler
**
** Default handler for the "mystery" interrupt that comes through vector
** 0x27.  This is a non-repeatable interrupt whose source has not been
** identified, but it appears to be the famous "spurious level 7 interrupt"
** source.
**
** @param vector   vector number for the interrupt that occurred
** @param code     error code, or a dummy value
*/
static void mystery_handler( int vector, int code ) {
#if defined(RPT_INT_MYSTERY) || defined(RPT_INT_UNEXP)
	cio_printf( "\nMystery interrupt!\nVector=0x%02x, code=%d\n",
		  vector, code );
#endif
	outb( PIC1_CMD, PIC_EOI );
}

/**
** init_pic
**
** Initialize the 8259 Programmable Interrupt Controller.
*/
static void init_pic( void ) {
	/*
	** ICW1: start the init sequence, update ICW4
	*/
	outb( PIC1_CMD, PIC_CW1_INIT | PIC_CW1_NEED4 );
	outb( PIC2_CMD, PIC_CW1_INIT | PIC_CW1_NEED4 );

	/*
	** ICW2: primary offset of 0x20 in the IDT, secondary offset of 0x28
	*/
	outb( PIC1_DATA, PIC1_CW2_VECBASE );
	outb( PIC2_DATA, PIC2_CW2_VECBASE );

	/*
	** ICW3: secondary attached to line 2 of primary, bit mask is 00000100
	**   secondary id is 2
	*/
	outb( PIC1_DATA, PIC1_CW3_SEC_IRQ2 );
	outb( PIC2_DATA, PIC2_CW3_SEC_ID );

	/*
	** ICW4: want 8086 mode, not 8080/8085 mode
	*/
	outb( PIC1_DATA, PIC_CW4_PM86 );
	outb( PIC2_DATA, PIC_CW4_PM86 );

	/*
	** OCW1: allow interrupts on all lines
	*/
	outb( PIC1_DATA, PIC_MASK_NONE );
	outb( PIC2_DATA, PIC_MASK_NONE );
}

/**
** set_idt_entry
**
** Construct an entry in the IDT
**
** @param entry    the vector number of the interrupt
** @param handler  ISR address to be put into the IDT entry
**
** Note: generally, the handler invoked from the IDT will be a "stub"
** that calls the second-level C handler via the isr_table array.
*/
static void set_idt_entry( int entry, void ( *handler )( void ) ) {
	IDT_Gate *g = (IDT_Gate *)IDT_ADDR + entry;

	g->offset_15_0 = (int)handler & 0xffff;
	g->segment_selector = 0x0010;
	g->flags = IDT_PRESENT | IDT_DPL_0 | IDT_INT32_GATE;
	g->offset_31_16 = (int)handler >> 16 & 0xffff;
}

/**
** Name:    init_idt
**
** Initialize the Interrupt Descriptor Table (IDT).  This makes each of
** the entries in the IDT point to the isr stub for that entry, and
** installs a default handler in the handler table.  Temporary handlers
** are then installed for those interrupts we may get before a real
** handler is set up.
*/
static void init_idt( void ) {
	int i;
	extern  void    ( *isr_stub_table[ 256 ] )( void );

	/*
	** Make each IDT entry point to the stub for that vector.  Also
	** make each entry in the ISR table point to the default handler.
	*/
	for ( i=0; i < 256; i++ ) {
		set_idt_entry( i, isr_stub_table[ i ] );
		install_isr( i, unexpected_handler );
	}

	/*
	** Install the handlers for interrupts that have (or will have) a
	** specific handler. Comments indicate which module init function
	** will eventually install the "real" handler.
	*/

	install_isr( VEC_KBD, default_handler );         // cio_init()
	install_isr( VEC_COM1, default_handler );        // sio_init()
	install_isr( VEC_TIMER, default_handler );       // clk_init()
	install_isr( VEC_SYSCALL, default_handler );     // sys_init()
	install_isr( VEC_PAGE_FAULT, default_handler );  // vm_init()

	install_isr( VEC_MYSTERY, mystery_handler );
}

/*
** END OF LOCAL ROUTINES.
**
** Full documentation for globally-visible routines is in the corresponding
** header file.
*/

/*
** panic
**
** Called when we find an unrecoverable error.
*/
void panic( char *reason ) {
	__asm__( "cli" );
	cio_printf( "\nPANIC: %s\nHalting...", reason );
	for(;;) {
		;
	}
}

/*
** init_interrupts
**
** (Re)initilizes the interrupt system.
*/
void init_interrupts( void ) {
	init_idt();
	init_pic();
}

/*
** install_isr
**
** Installs a second-level handler for a specific interrupt.
*/
void (*install_isr( int vector,
		void (*handler)(int,int) ) ) ( int, int ) {

	void ( *old_handler )( int vector, int code );

	old_handler = isr_table[ vector ];
	isr_table[ vector ] = handler;
	return old_handler;
}

/*
** Name:	delay
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
** Ultimately, just remember that DELAY VALUES ARE APPROXIMATE AT BEST.
*/
void delay( int length ) {

	while( --length >= 0 ) {
		for( int i = 0; i < 10000000; ++i )
			;
	}
}
