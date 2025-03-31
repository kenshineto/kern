/**
** @file	user.h
**
** @author	CSCI-452 class of 20245
**
** @brief	Declarations of user-level code management routines
*/

#ifndef USER_H_
#define USER_H_

#include <common.h>

#include <procs.h>
#include <x86/arch.h>

// default value for EFLAGS in new processes
#define DEFAULT_EFLAGS (EFL_MB1 | EFL_IF)

/*
** General (C and/or assembly) definitions
*/

#ifndef ASM_SRC

/*
** Start of C-only definitions
*/

/*
** Types
*/

/*
** Blob file organization
**
** The file begins with a four-byte magic number and a four-byte integer
** indicating the number of ELF files contained in the blob. This is
** followed by an array of 32-byte file table entries, and then the contents
** of the ELF files in the order they appear in the program file table.
**
**		Bytes        Contents
**		-----        ----------------------------
**		0 - 3        File magic number ("BLB\0")
**      4 - 7        Number of ELF files in blob ("n")
**      8 - n*32+8   Program file table
**		n*32+9 - ?   ELF file contents
**
** Each program file table entry contains the following information:
**
**		name         File name (up to 19 characters long)
**		offset       Byte offset to the ELF header for this file
**		size         Size of this ELF file, in bytes
**		flags        Flags related to this file
*/

// user program blob header
typedef struct header_s {
	char magic[4];
	uint32_t num;
} header_t;

// length of the file name field
#define NAMELEN 20

// program descriptor
typedef struct prog_s {
	char name[NAMELEN]; // truncated name (15 chars)
	uint32_t offset; // offset from the beginning of the blob
	uint32_t size; // size of this ELF module
	uint32_t flags; // miscellaneous flags
} prog_t;

/*
** Globals
*/

/*
** Prototypes
*/

/**
** Name:	user_init
**
** Initializes the user support module.
*/
void user_init(void);

/**
** Name:	user_locate
**
** Locates a user program in the user code archive.
**
** @param what   The ID of the user program to find
**
** @return pointer to the program table entry in the code archive, or NULL
*/
prog_t *user_locate(uint_t what);

/**
** Name:	user_duplicate
**
** Duplicates the memory setup for an existing process.
**
** @param new   The PCB for the new copy of the program
** @param old   The PCB for the existing the program
**
** @return the status of the duplicate attempt
*/
int user_duplicate(pcb_t *new, pcb_t *old);

/**
** Name:	user_load
**
** Loads a user program from the user code archive into memory.
** Allocates all needed frames and sets up the VM tables.
**
** @param prog   A pointer to the program table entry to be loaded
** @param pcb    The PCB for the program being loaded
** @param args   The argument vector for the program
** @param sys    Is the argument vector from kernel code?
**
** @return the status of the load attempt
*/
int user_load(prog_t *prog, pcb_t *pcb, const char **args, bool_t sys);

/**
** Name:	user_cleanup
**
** "Unloads" a user program. Deallocates all memory frames and
** cleans up the VM structures.
**
** @param pcb   The PCB of the program to be cleaned up
*/
void user_cleanup(pcb_t *pcb);

#endif /* !ASM_SRC */

#endif
