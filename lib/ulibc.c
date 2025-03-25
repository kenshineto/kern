/**
** @file	ulibc.c
**
** @author	CSCI-452 class of 20245
**
** @brief	C implementations of user-level library functions
*/

#include <common.h>

/*
** PRIVATE DEFINITIONS
*/

/*
** PRIVATE DATA TYPES
*/

/*
** PRIVATE GLOBAL VARIABLES
*/

/*
** PUBLIC GLOBAL VARIABLES
*/

/*
** PRIVATE FUNCTIONS
*/

/*
** PUBLIC FUNCTIONS
*/

/*
**********************************************
** CONVENIENT "SHORTHAND" VERSIONS OF SYSCALLS
**********************************************
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
** @returns The PID of the terminated child, or an error code
*/
int wait( int32_t *status ) {
	return( waitpid(0,status) );
}

/**
** spawn - create a new process running a different program
**
** usage:   pid = spawn(what,args);
**
** Creates a new process and then execs 'what'
**
** @param what  The program table index of the program to spawn
** @param args  The command-line argument vector for the new process
**
** @returns PID of the new process, or an error code
*/
int32_t spawn( uint_t what, char **args ) {
	int32_t pid;
	char buf[256];

	pid = fork();
	if( pid != 0 ) {
		// failure, or we are the parent
		return( pid );
	}

	// we are the child
	pid = getpid();

	// child inherits parent's priority level

	exec( what, args );

	// uh-oh....

	sprint( buf, "Child %d exec() #%u failed\n", pid, what );
	cwrites( buf );

	exit( EXIT_FAILURE );
	return( 0 );   // shut the compiler up
}

/**
** cwritech(ch) - write a single character to the console
**
** @param ch The character to write
**
** @returns The return value from calling write()
*/
int cwritech( char ch ) {
	return( write(CHAN_CIO,&ch,1) );
}

/**
** cwrites(str) - write a NUL-terminated string to the console
**
** @param str The string to write
**
*/
int cwrites( const char *str ) {
	int len = strlen(str);
	return( write(CHAN_CIO,str,len) );
}

/**
** cwrite(buf,size) - write a sized buffer to the console
**
** @param buf  The buffer to write
** @param size The number of bytes to write
**
** @returns The return value from calling write()
*/
int cwrite( const char *buf, uint32_t size ) {
	return( write(CHAN_CIO,buf,size) );
}

/**
** swritech(ch) - write a single character to the SIO
**
** @param ch The character to write
**
** @returns The return value from calling write()
*/
int swritech( char ch ) {
	return( write(CHAN_SIO,&ch,1) );
}

/**
** swrites(str) - write a NUL-terminated string to the SIO
**
** @param str The string to write
**
** @returns The return value from calling write()
*/
int swrites( const char *str ) {
	int len = strlen(str);
	return( write(CHAN_SIO,str,len) );
}

/**
** swrite(buf,size) - write a sized buffer to the SIO
**
** @param buf  The buffer to write
** @param size The number of bytes to write
**
** @returns The return value from calling write()
*/
int swrite( const char *buf, uint32_t size ) {
	return( write(CHAN_SIO,buf,size) );
}
