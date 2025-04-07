#include <comus/drivers.h>
#include <comus/drivers/uart.h>
#include <comus/drivers/tty.h>
#include <comus/drivers/pci.h>
#include <comus/drivers/clock.h>

void drivers_init(void)
{
	uart_init();
	pci_init();
	clock_update();
}
