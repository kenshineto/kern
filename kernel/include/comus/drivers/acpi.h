/**
 * @file acpi.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * ACPI definitions
 */

/**
 * Loads the ACPI tables
 * https://en.wikipedia.org/wiki/ACPI
 * @param rsdp - pointer to the Root System Description Pointer
 * usually passed from the bootloader
 */
void acpi_init(void *rsdp);

/**
 * Shutdowns down the system
 */
__attribute__((noreturn)) void acpi_shutdown(void);
