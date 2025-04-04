#include <comus/drivers.h>
#include <comus/drivers/pci.h>

void drivers_init(void)
{
	pci_init();
}
