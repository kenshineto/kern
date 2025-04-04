
#include "fpu.h"
#include "pic.h"
#include "idt.h"

void cpu_init(void) {
	pic_remap();
	idt_init();
	fpu_init();
}
