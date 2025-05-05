#include <lib.h>
#include <stdarg.h>
#include <comus/asm.h>
#include <comus/drivers/ps2.h>
#include <comus/drivers/spkr.h>

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

	fatal_loop();
}

__attribute__((noreturn)) void fatal_loop(void)
{
	while(1) {
		spkr_play_tone(1000);
		ps2_set_leds(0x4);
		kspin_milliseconds(200);
		spkr_quiet();
		ps2_set_leds(0x0);
		kspin_milliseconds(800);
	}
}
