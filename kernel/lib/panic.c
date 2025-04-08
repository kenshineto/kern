#include <lib.h>
#include <stdarg.h>
#include <comus/asm.h>

__attribute__((noreturn)) void panic(const char *format, ...)
{
	cli();
	va_list list;
	va_start(list, format);
	kprintf("\n\n!!! PANIC !!!\n");
	kvprintf(format, list);
	kprintf("\n\n");

	while (1)
		halt();
}
