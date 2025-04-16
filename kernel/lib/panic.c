#include <lib.h>
#include <stdarg.h>
#include <comus/asm.h>

__attribute__((noreturn)) void __panic(const char *line, const char *file,
									   const char *format, ...)
{
	cli();
	va_list list;
	va_start(list, format);
	kprintf("\n\n!!! PANIC !!!\n");
	kprintf("In file %s at line %s:\n", file, line);
	kvprintf(format, list);
	kprintf("\n\n");
	log_backtrace();

	while (1)
		halt();
}
