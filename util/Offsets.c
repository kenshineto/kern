/*
** SCCS ID:	@(#)Offsets.c	2.2	1/23/25
**
** @file	Offsets.c
**
** @author	Warren R. Carithers
**
** Print byte offsets for fields in various structures.
**
** This program exists to simplify life.  If/when fields in a structure
** are changed, this can be modified, recompiled and executed to come up with
** byte offsets for use in accessing structure fields from assembly language.
** It makes use of the C 'offsetof' macro (defined since C89).
**
** IMPORTANT NOTE:  compiling this on a 64-bit architecture will yield
** incorrect results by default, as 64-bit GCC versions most often use
** the LP64 model (longs and pointers are 64 bits).  Add the "-mx32"
** option to the compiler (compile for x86_64, but use 32-bit sizes),
** and make sure you have the 'libc6-dev-i386' package installed (for
** Ubuntu systems).
**
** WATCH FOR clashes with standard system functions; compile with the
** "-fno-builtin" option to avoid warnings about clashes with builtin
** function declarations.
**
** Writes to stdout. Output contains information about type sizes and
** byte offsets for fields within structures.  If invoked with the -h
** option, the output takes the form of a standard C header file; 
** otherwise, the information is printed in ordinary text form.
**
** If compiled with the CREATE_HEADER_FILE macro defined, when invoked
** with the -h option, writes the data directly into a file named
** "offsets.h".
*/

// make sure we get all the kernel stuff
#define KERNEL_SRC

// include any of our headers that define data structures to be described
#include <common.h>
#include <procs.h>

// avoid complaints about NULL from stdio.h
#ifdef NULL
#undef NULL
#endif

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

/*
** We don't include <time.h> because of conflicts with a time_t data
** type we may be defining; instead, we provide our own prototypes for
** ctime() and time().
*/

extern char *ctime( const __time_t *timep );
extern __time_t time( __time_t *tloc );

/*
** Header comment, including header guard
**
** No newline on the "Creation date" line, because ctime()
** puts a newline at the end of the string it produces.
*/
char h_prefix[] = "/**\n"
"** @file\toffsets.h\n"
"**\n"
"** GENERATED AUTOMATICALLY - DO NOT EDIT\n"
"**\n"
"** Creation date: %s"
"**\n"
"** This header file contains C Preprocessor macros which expand\n"
"** into the byte offsets needed to reach fields within structs\n"
"** used in the baseline system.  Should those struct declarations\n"
"** change, the Offsets program should be modified (if needed),\n"
"** recompiled, and re-run to recreate this file.\n"
"*/\n"
"\n"
"#ifndef OFFSETS_H_\n"
"#define OFFSETS_H_\n";

/*
** Header guard suffix
*/
char h_suffix[] = "\n"
"#endif\n";

// are we generating the .h file?
int genheader = 0;

// header file stream
FILE *hfile;

// prefix for header file lines

// produce a report line
void process( const char *sname, const char *field, size_t bytes ) {
	if( genheader ) {
		char name[64];
		sprintf( name, "%s_%s", sname, field );
		fprintf( hfile, "#define\t%-23s\t%u\n", name, bytes );
	} else {
		printf( "  %-10s  %u\n", field, bytes );
	}
}

#ifdef CREATE_HEADER_FILE
// dump out the header
void setheader( void ) {
	// trigger output into the header file
	genheader = 1;

	hfile = fopen( "offsets.h", "w" );
	if( hfile == NULL ) {
		perror( "offsets.h" );
		exit( 1 );
	}

	__time_t t;
	(void) time( &t );

	fprintf( hfile, h_prefix, ctime(&t) );
}
#endif  /* CREATE_HEADER_FILE */

// introduce an "offsets" section for structs
void hsection( const char *name, const char *typename, size_t size ) {
	if( genheader ) {
		fprintf( hfile, "\n// %s structure\n\n", typename );
		process( "SZ", name, size );
		fputc( '\n', hfile );
	} else {
		printf( "Offsets into %s (%u bytes):\n", typename, size );
	}
}

// introduce a "sizes" section for types
void tsection( const char *name, const char *typename ) {
	if( genheader ) {
		fprintf( hfile, "\n// Sizes of %s types\n\n", typename );
	} else {
		printf( "Sizes of %s types:\n", typename );
	}
}

int main( int argc, char *argv[] ) {

	hfile = stdout;

	if( argc > 1 ) {
#ifdef CREATE_HEADER_FILE
		// only accept one argument
		if( argc == 2 ) {
			// -h:  produce an offsets.h header file
			if( argv[1][0] == '-' && argv[1][1] == '-h' ) {
				setheader();
			}
		} else {
			fprintf( stderr, "usage: %s [-h]\n", argv[0] );
			exit( 1 );
		}
#else
		// we accept no arguments
		fprintf( stderr, "usage: %s\n", argv[0] );
		exit( 1 );
#endif  /* CREATE_HEADER_FILE */
	}

	/*
	** Basic and simple/opaque types
	*/

	tsection( "SZ", "basic" );
	process( "SZ", "char", sizeof(char) );
	process( "SZ", "short", sizeof(short) );
	process( "SZ", "int", sizeof(int) );
	process( "SZ", "long", sizeof(long) );
	process( "SZ", "long_long", sizeof(long long) );
	process( "SZ", "pointer", sizeof(void *) );
	fputc( '\n', hfile );

	tsection( "SZ", "our" );
	process( "SZ", "int8_t", sizeof(int8_t) );
	process( "SZ", "uint8_t", sizeof(uint8_t) );
	process( "SZ", "int16_t", sizeof(int16_t) );
	process( "SZ", "uint16_t", sizeof(uint16_t) );
	process( "SZ", "int32_t", sizeof(int32_t) );
	process( "SZ", "uint32_t", sizeof(uint32_t) );
	process( "SZ", "int64_t", sizeof(int64_t) );
	process( "SZ", "uint64_t", sizeof(uint64_t) );
	process( "SZ", "bool_t", sizeof(bool_t) );
	fputc( '\n', hfile );

	/*
	** Structured types whose fields we are describing
	*/

	// add entries for each type here, as needed

	hsection( "CTX", "context_t", sizeof(context_t) );
	process( "CTX", "ss", offsetof(context_t,ss) );
	process( "CTX", "gs", offsetof(context_t,gs) );
	process( "CTX", "fs", offsetof(context_t,fs) );
	process( "CTX", "es", offsetof(context_t,es) );
	process( "CTX", "ds", offsetof(context_t,ds) );
	process( "CTX", "edi", offsetof(context_t,edi) );
	process( "CTX", "esi", offsetof(context_t,esi) );
	process( "CTX", "ebp", offsetof(context_t,ebp) );
	process( "CTX", "esp", offsetof(context_t,esp) );
	process( "CTX", "ebx", offsetof(context_t,ebx) );
	process( "CTX", "edx", offsetof(context_t,edx) );
	process( "CTX", "ecx", offsetof(context_t,ecx) );
	process( "CTX", "eax", offsetof(context_t,eax) );
	process( "CTX", "vector", offsetof(context_t,vector) );
	process( "CTX", "code", offsetof(context_t,code) );
	process( "CTX", "eip", offsetof(context_t,eip) );
	process( "CTX", "cs", offsetof(context_t,cs) );
	process( "CTX", "eflags", offsetof(context_t,eflags) );
	fputc( '\n', hfile );

	hsection( "SCT", "section_t", sizeof(section_t) );
	process( "SCT", "length", offsetof(section_t,length) );
	process( "SCT", "addr", offsetof(section_t,addr) );
	fputc( '\n', hfile );

	hsection( "PCB", "pcb_t", sizeof(pcb_t) );
	process( "PCB", "context", offsetof(pcb_t,context) );
	process( "PCB", "next", offsetof(pcb_t,next) );
	process( "PCB", "parent", offsetof(pcb_t,parent) );
	process( "PCB", "wakeup", offsetof(pcb_t,wakeup) );
	process( "PCB", "exit_status", offsetof(pcb_t,exit_status) );
	process( "PCB", "pdir", offsetof(pcb_t,pdir) );
	process( "PCB", "sects", offsetof(pcb_t,sects) );
	process( "PCB", "pid", offsetof(pcb_t,pid) );
	process( "PCB", "state", offsetof(pcb_t,state) );
	process( "PCB", "priority", offsetof(pcb_t,priority) );
	process( "PCB", "ticks", offsetof(pcb_t,ticks) );
	fputc( '\n', hfile );

	// finish up the offsets.h file if we need to
	if( genheader ) {
		fputs( h_suffix, hfile );
		fclose( hfile );
	}

	return( 0 );
}
