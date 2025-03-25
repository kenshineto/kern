/**
** @file	ulibs.S
**
** @author	CSCI-452 class of 20245
**
** @brief	assembly-language user-level library functions
*/

#define	ASM_SRC

// get the system call codes

#include <syscalls.h>

/**
** System call stubs
**
** All have the same structure:
**
**      move a code into EAX
**      generate the interrupt
**      return to the caller
**
** As these are simple "leaf" routines, we don't use
** the standard enter/leave method to set up a stack
** frame - that takes time, and we don't really need it.
**
** Could be modified to use the UNIX/Linux convention of
** having the syscall code set the 'C' flag to indicate that
** the value being returned in %EAX is an error code:
**
**	  ...
**	int	$VEC_SYSCALL
**	jc	set_errno
**	ret
**	  ...
**
**	.globl	errno
** set_errno:
**	movl	%eax, errno
**	movl	$-1, %eax
**	ret
*/

#define	SYSCALL(name) \
	.globl	name			; \
name:					; \
	movl	$SYS_##name, %eax	; \
	int	$VEC_SYSCALL	; \
	ret

/*
** "real" system calls
*/

SYSCALL(exit)
SYSCALL(waitpid)
SYSCALL(fork)
SYSCALL(exec)
SYSCALL(read)
SYSCALL(write)
SYSCALL(getpid)
SYSCALL(getppid)
SYSCALL(gettime)
SYSCALL(getprio)
SYSCALL(setprio)
SYSCALL(kill)
SYSCALL(sleep)

/*
** This is a bogus system call; it's here so that we can test
** our handling of out-of-range syscall codes in the syscall ISR.
*/
SYSCALL(bogus)

/*
** Other library functions
*/

/**
** fake_exit()
**
** Dummy "startup" function
**
** calls exit(%eax) - serves as the "return to" code for
** main() functions, in case they don't call exit() themselves
*/

	.globl	fake_exit
fake_exit:
	// alternate: could push a "fake exit" status
	pushl	%eax	// termination status returned by main()
	call	exit	// terminate this process
