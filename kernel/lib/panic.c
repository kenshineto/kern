#include <lib.h>
#include <stdarg.h>
#include <comus/asm.h>

__attribute__((noreturn)) void __panic(unsigned int line, const char *file,
									   const char *format, ...)
{
	cli();
#if LOG_LEVEL >= LOG_LVL_PANIC
	va_list list;
	va_start(list, format);
	kprintf("\n\n!!! PANIC !!!\n");
	kprintf("In file %s at line %d:\n", file, line);
	kvprintf(format, list);
	kprintf("\n\n");
	log_backtrace();
#endif

	while (1)
		halt();
}
