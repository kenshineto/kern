	.globl _start
	.extern main
	.extern exit

	.section .text
	.code64
_start:
	call main
	subq	$16, %rsp # ???
	pushq	%rax
	call	exit
