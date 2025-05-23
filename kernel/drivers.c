#include <comus/drivers.h>
#include <comus/drivers/acpi.h>
#include <comus/drivers/uart.h>
#include <comus/drivers/ps2.h>
#include <comus/drivers/pci.h>
#include <comus/drivers/ata.h>
#include <comus/drivers/gpu.h>
#include <comus/drivers/pit.h>
#include <comus/mboot.h>

void drivers_init(void)
{
	pit_set_freq(CHAN_TIMER, 1000); // 1ms
	uart_init();
	ps2_init();
	pci_init();
	ata_init();
	acpi_init(mboot_get_rsdp());
	gpu_init();
}
