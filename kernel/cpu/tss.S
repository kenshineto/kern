	.globl tss_flush

tss_flush:
	movw	$0x28, %ax
	ltr		%ax
	ret
