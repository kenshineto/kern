/**
 * @file unistd.h
 * @author Freya Murphy <freya@freyacat.org>
 * @author Warren R. Carithers
 *
 * Universial system definitions for userspace.
 */

#ifndef UNISTD_H_
#define UNISTD_H_

#include <stdint.h>
#include <stddef.h>

/* System Call Definitions */

typedef unsigned short pid_t;

/**
 * terminates the calling process and does not return.
 *
 * @param status - the status code to exit with
 */
__attribute__((noreturn)) extern void exit(int status);

/**
 * sleeps current process until a child process terminates
 *
 * @param pid - pid of the desired child, or 0 for any child
 * @param status - pointer to int32_t into which the child's status is placed,
 *                 or NULL
 * @return The pid of the terminated child, or an error code
 *
 * If there are no children in the system, returns an error code (*status
 * is unchanged).
 *
 * If there are one or more children in the system and at least one has
 * terminated but hasn't yet been cleaned up, cleans up that process and
 * returns its information; otherwise, blocks until a child terminates.
 */
extern int waitpid(pid_t pid, int *status);

/**
 * create a duplicate of the calling process
 *
 * @return parent - the pid of the new child, or an error code
 *         child  - 0
 */
extern int fork(void);

/**
 * replace the memory image of the calling process
 *
 * @param prog - program table index of the program to exec
 * @param args - the command-line argument vector
 *
 * Does not return if it succeeds; if it returns, something has
 * gone wrong.
 */
extern void exec(const char *filename, char **args);

/**
 * open a stream with a given filename
 *
 * @param filename - the name of the file to open
 * @return the file descriptior of the open file or a negative error code.
 */
extern int open(const char *filename);

/**
 * closes a stream with the given file descriptior
 *
 * @param fd - the file descriptior of the open stream
 */
extern void close(int fd);

/**
 * read into a buffer from a stream
 *
 * @param fd - file stream to read from
 * @param buf - buffer to read into
 * @param nbytes - maximum capacity of the buffer
 * @return - The count of bytes transferred, or an error code
 */
extern int read(int fd, void *buffer, size_t nbytes);

/**
 * write from a buffer to a stream
 *
 * @param fd - file stream to write to
 * @param buf - buffer to write from
 * @param nbytes - maximum capacity of the buffer
 * @return - The count of bytes transferred, or an error code
 */
extern int write(int fd, const void *buffer, size_t nbytes);

/**
 * gets the pid of the calling process
 *
 * @return the pid of this process
 */
extern pid_t getpid(void);

/**
 * gets the parent pid of the current process
 *
 * @return the parent pid of the current process, or 0 if init
 */
extern pid_t getppid(void);

/**
 * gets the current system time
 *
 * @return the system time
 */
extern unsigned long gettime(void);

/**
 * gets the scheduling priority of the calling process
 *
 * @return the process' priority
 */
extern unsigned int getprio(void);

/**
 * sets the scheduling priority of the calling process
 *
 * @param new - the desired new priority
 * @return the old priority value
 */
extern unsigned int setprio(unsigned int new);

/**
 * terminates a process
 *
 * @param pid - the pid of the process to kill
 * @return 0 on success, else an error code
 */
extern int kill(pid_t pid);

/**
 * put the current process to sleep for some length of time
 *
 * @param ms - desired sleep time (in ms), or 0 to yield the CPU
 * @return the time the process spent sleeping (in ms)
 */
extern int sleep(unsigned long ms);

/**
 * Wait for any child to exit
 *
 * @param status - pointer to int32_t into which the child's status is placed,
 *                 or NULL
 * @return  The pid of the terminated child, or an error code
 *
 * Analogous to waitpid(0,status)
 */
extern int wait(int *status);

/**
 * Spawn a new process running a different program
 *
 * @param prog - program table index of the program to spawn
 * @param args - the command-line argument vector for the process
 * @return the pid of the child, or an error code
 *
 * Analogous to calling fork and exec
 */
extern int spawn(pid_t prog, char **args);

/**
 * Increment the program's data space by increment bytes.
 *
 * @param increment - the amount in bytes to increment the heap
 * @return the previos program break on success, or NULL on failure
 *
 */
extern void *sbrk(intptr_t increment);

#endif /* unistd.h */
