/**
 * @file syscalls.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 * @author cisi 452
 *
 * System call declarations
 */

#ifndef SYSCALLS_H_
#define SYSCALLS_H_

#define SYS_exit 0
#define SYS_waitpid 1
#define SYS_fork 2
#define SYS_exec 3
#define SYS_open 4
#define SYS_close 5
#define SYS_read 6
#define SYS_write 7
#define SYS_getpid 8
#define SYS_getppid 9
#define SYS_gettime 10
#define SYS_getprio 11
#define SYS_setprio 12
#define SYS_kill 13
#define SYS_sleep 14
#define SYS_brk 15
#define SYS_sbrk 16
#define SYS_poweroff 17
#define SYS_drm 18
#define SYS_ticks 19
#define SYS_allocshared 20
#define SYS_popsharedmem 21

// UPDATE THIS DEFINITION IF MORE SYSCALLS ARE ADDED!
#define N_SYSCALLS 22

// interrupt vector entry for system calls
#define VEC_SYSCALL 0x80

#endif /* syscalls.h */
