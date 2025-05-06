	.globl _start
	.extern main
	.extern exit

	.section .text
	.code64
_start:
	call	main
	movq	%rax, %rdi
	call	exit
