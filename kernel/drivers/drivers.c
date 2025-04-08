#include <comus/drivers.h>
#include <comus/drivers/uart.h>
#include <comus/drivers/pci.h>

void drivers_init(void)
{
	uart_init();
	pci_init();
}
