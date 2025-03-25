/**
** @file	syscalls.h
**
** @author	CSCI-452 class of 20245
**
** @brief	System call declarations
*/

#ifndef SYSCALLS_H_
#define SYSCALLS_H_

#include <common.h>

/*
** General (C and/or assembly) definitions
*/

/*
** system call codes
**
** these are used in the user-level C library stub functions,
** and are defined here as CPP macros instead of as an enum
** so that they can be used from assembly
*/

#define SYS_exit        0
#define SYS_waitpid     1
#define SYS_fork        2
#define SYS_exec        3
#define SYS_read        4
#define SYS_write       5
#define SYS_getpid      6
#define SYS_getppid     7
#define SYS_gettime     8
#define SYS_getprio     9
#define SYS_setprio     10
#define SYS_kill        11
#define SYS_sleep       12

// UPDATE THIS DEFINITION IF MORE SYSCALLS ARE ADDED!
#define N_SYSCALLS      13

// dummy system call code for testing our ISR
#define SYS_bogus       0xbad

// interrupt vector entry for system calls
#define VEC_SYSCALL     0x80

#ifndef ASM_SRC

/*
** Start of C-only definitions
*/

/*
** Types
*/

/*
** Globals
*/

/*
** Prototypes
*/

#ifdef KERNEL_SRC

/**
** Name:  sys_init
**
** Syscall module initialization routine
*/
void sys_init( void );

#endif  /* KERNEL_SRC */

#endif  /* !ASM_SRC */

#endif
