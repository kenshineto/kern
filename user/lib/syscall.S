#include <syscalls.h>

.macro SYSCALL name num
	.align 8
	.globl \name
\name:
	movq	$\num, %rax
	int		$VEC_SYSCALL
	ret
.endm

SYSCALL exit SYS_exit
SYSCALL waitpid SYS_waitpid
SYSCALL fork SYS_fork
SYSCALL exec SYS_exec
SYSCALL read SYS_read
SYSCALL write SYS_write
SYSCALL getpid SYS_getpid
SYSCALL getppid SYS_getppid
SYSCALL gettime SYS_gettime
SYSCALL getprio SYS_getprio
SYSCALL setprio SYS_setprio
SYSCALL kill SYS_kill
SYSCALL sleep SYS_sleep
SYSCALL brk SYS_brk
SYSCALL sbrk SYS_sbrk
SYSCALL poweroff SYS_poweroff
SYSCALL drm SYS_drm
SYSCALL ticks SYS_ticks
