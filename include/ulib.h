/**
** @file	ulib.h
**
** @author	CSCI-452 class of 20245
**
** @brief	Declarations for user-level library functions
**
** This module implements a simple collection of support functions
** similar to the standard C library.
*/

#ifndef ULIB_H_
#define ULIB_H_

#include <common.h>

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
** Globals
*/

/*
** Prototypes
*/

/*
*************************************************
** SYSTEM CALLS *********************************
*************************************************
*/

/**
** exit - terminate the calling process
**
** usage:   exit(status);
**
** @param status   Termination status of this process
**
** Does not return.
*/
void exit( int32_t status );

/**
** waitpid - wait for a child process to terminate
**
** usage:   pid = waitpid(pid,&status);
**
** @param pid    PID of the desired child, or 0 for any child
** @param status Pointer to int32_t into which the child's status is placed,
**               or NULL
**
** @return  The PID of the terminated child, or an error code
**
** If there are no children in the system, returns an error code (*status
** is unchanged).
**
** If there are one or more children in the system and at least one has
** terminated but hasn't yet been cleaned up, cleans up that process and
** returns its information; otherwise, blocks until a child terminates.
*/
int waitpid( uint_t pid, int32_t *status );

/**
** fork - create a duplicate of the calling process
**
** usage:   pid = fork();
**
** @return   parent - the pid of the new child, or an error code
**           child  - 0
*/
int fork( void );

/**
** exec - replace the memory image of the calling process
**
** usage:   exec( what, args )
**
** @param what    program table index of the program to exec
** @param args    the command-line argument vector
**
** Does not return if it succeeds; if it returns, something has
** gone wrong.
*/
void exec( uint_t what, char **args );

/**
** read - read into a buffer from a stream
**
** usage:   n = read(channel,buf,length)
**
** @param chan   I/O stream to read from
** @param buf    Buffer to read into
** @param length Maximum capacity of the buffer
**
** @return   The count of bytes transferred, or an error code
*/
int read( uint_t chan, void *buffer, uint_t length );

/**
** write - write from a buffer to a stream
**
** usage:   n = write(channel,buf,length)
**
** @param chan   I/O stream to write to
** @param buf    Buffer to write from
** @param length Maximum capacity of the buffer
**
** @return   The count of bytes transferred, or an error code
*/
int write( uint_t chan, const void *buffer, uint_t length );

/**
** getpid - get the PID of the calling process
**
** usage:   pid = getpid()
**
** @return   the PID of this process
*/
uint_t getpid( void );

/**
** getppid - get the PID of the calling process' parent
**
** usage:   pid = getppid()
**
** @return   the PID of this process' parent
*/
uint_t getppid( void );

/**
** gettime - get the current system time
**
** usage:   pid = gettime()
**
** @return   the system time
*/
uint32_t gettime( void );

/**
** getprio - get the scheduling priority of the calling process
**
** usage:   prio = getprio()
**
** @return   the process' priority
*/
int getprio( void );

/**
** setprio - set the scheduling priority of the calling process
**
** usage:   oldprio = setprio(newprio)
**
** @param new   the desired new priority
**
** @return   the old priority value
*/
int setprio( int new );

/**
** kill - terminate a process with extreme prejudice
**
** usage:   n = kill( pid )
**
** @param pid  the intended victim
**
** @return   0 on success, else an error code
*/
int32_t kill( uint_t pid );

/**
** sleep - put the current process to sleep for some length of time
**
** usage:   sleep(n);
**
** @param ms Desired sleep time (in ms), or 0 to yield the CPU
**
** @return the time the process spent sleeping (in ms)
*/
int sleep( uint32_t ms );

/**
** bogus - a nonexistent system call, to test our syscall ISR
**
** usage:   bogus()
**
** Does not return.
*/
void bogus( void );

/*
*************************************************
** CONVENIENT "SHORTHAND" VERSIONS OF SYSCALLS **
*************************************************
**
** These are library functions that perform specific common
** variants of system calls. This helps reduce the total number
** of system calls, keeping our baseline OS as lean and mean
** as we can make it. :-)
*/

/**
** wait - wait for any child to exit
**
** usage:   pid = wait(&status)
**
** Calls waitpid(0,status)
**
** @param status Pointer to int32_t into which the child's status is placed,
**               or NULL
**
** @return  The PID of the terminated child, or an error code
*/
int wait( int32_t *status );

/**
** spawn - create a new process running a different program
**
** usage:       n = spawn(what,args)
**
** Creates a new process and then execs 'what'
**
** @param what  Program table index of the program to spawn
** @param args  The command-line argument vector for the process
**
** @return      The PID of the child, or an error code
*/
int spawn( uint_t what, char **args );

/**
** cwritech(ch) - write a single character to the console
**
** @param ch The character to write
**
** @return  The return value from calling write()
*/
int cwritech( char ch );

/**
** cwrites(str) - write a NUL-terminated string to the console
**
** @param str The string to write
**
*/
int cwrites( const char *str );

/**
** cwrite(buf,leng) - write a sized buffer to the console
**
** @param buf  The buffer to write
** @param leng The number of bytes to write
**
** @return  The return value from calling write()
*/
int cwrite( const char *buf, uint32_t leng );

/**
** swritech(ch) - write a single character to the SIO
**
** @param ch The character to write
**
** @return  The return value from calling write()
*/
int swritech( char ch );

/**
** swrites(str) - write a NUL-terminated string to the SIO
**
** @param str The string to write
**
** @return  The return value from calling write()
*/
int swrites( const char *str );

/**
** swrite(buf,leng) - write a sized buffer to the SIO
**
** @param buf  The buffer to write
** @param leng The number of bytes to write
**
** @return  The return value from calling write()
*/
int swrite( const char *buf, uint32_t leng );

/*
*************************************************
** MISCELLANEOUS USEFUL SUPPORT FUNCTIONS *******
*************************************************
*/

/**
** fake_exit()
**
** dummy "startup" function
**
** calls exit(%eax) - serves as the "return to" code for
** main() functions, in case they don't call exit() themselves
*/
void fake_exit( void );

#endif /* !ASM_SRC */

#endif
