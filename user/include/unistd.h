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
#include <sys/types.h>

/* System Call Definitions */

// NOTE: needs to match kernel input.h
struct keycode {
	char key;
	char flags;
};

typedef unsigned short pid_t;

enum {
	S_SET = 0,
	S_CUR = 1,
	S_END = 2,
};

enum {
	O_CREATE = 0x01,
	O_RDONLY = 0x02,
	O_WRONLY = 0x04,
	O_APPEND = 0x08,
	O_RDWR = 0x010,
};

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
 * @returns error code on failure
 */
extern int exec(const char *filename, const char **args);

/**
 * open a stream with a given filename
 *
 * @param filename - the name of the file to open
 * @return the file descriptior of the open file or a negative error code.
 */
extern int open(const char *filename, int flags);

/**
 * closes a stream with the given file descriptior
 *
 * @param fd - the file descriptior of the open stream
 * @returns 0 on success, error code on invalid fd
 */
extern int close(int fd);

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
 * seek a file
 *
 * @param fd - file stream to seek
 * @param off - offset to seek
 * @param whence - whence to seek
 * @return 0 on success, or an error code
 */
extern off_t seek(int fd, off_t off, int whence);

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
 * Set the heap break to addr
 *
 * @param addr - sets the programs break to addr
 * @return the previos program break on success, or NULL on failure
 */
extern void *brk(const void *addr);

/**
 * Increment the program's data space by increment bytes.
 *
 * @param increment - the amount in bytes to increment the heap
 * @return the previos program break on success, or NULL on failure
 */
extern void *sbrk(intptr_t increment);

/**
 * Allocate a number of pages shared with another PID. Does not map the pages
 * into the other process's vtable until the other process calls popsharedmem().
 *
 * @param num_pages number of pages to allocate
 * @param other_pid pid of other process
 * @return pointer to the virtual address which will be accessible by both,
 * after popsharedmem() is called.
 */
extern void *allocshared(size_t num_pages, int other_pid);

/**
 * Checks if another process has tried to share memory with us, and return it.
 * No size information is returned, it is only guaranteed that there is at least
 * one page in the shared allocation. To get around this, the sharer can write
 * a size number to the start of the first page.
 *
 * @return page aligned pointer to the start of the shared pages, or NULL if no
 * process has tried to share with us, or NULL if we the shared virtual address
 * space is already occupied in the caller's pagetable.
 */
extern void *popsharedmem(void);

/**
 * Get the most recent key event, if there is one.
 *
 * @param poll the keycode to write out to
 * @return 0 if there was no key event, in which case `poll` was not changed, or
 * 1 if there was an event and the caller should read from `poll`.
 */
extern int keypoll(struct keycode *poll);

/**
 * Poweroff the system.
 *
 * @return 1 on failure
 */
extern int poweroff(void);

/**
 * Gets access to the "direct rendering manager" (framebuffer)
 *
 * @param fb - address of framebuffer
 * @param width - returns the width of the framebuffer
 * @param height - returns the height of the framebuffer
 * @param bpp - returns the bit depth of the framebuffer
 * @returns 0 on success, error code on failure
 */
extern int drm(void **fb, int *width, int *height, int *bbp);

/**
 * @returns number of ticks the system has been up for (1ms)
 */
extern uint64_t ticks(void);

#endif /* unistd.h */
