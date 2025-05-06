# Drivers

All drivers and their uses

## acpi.c

ACPI (Advanced Configuration and Power Interface)
- allows powering off the system

## ata.c

ATA (Advanced Technology Attachment) / IDE (Integrated Drive Electronics)

This driver exposes a few functions. Because devices are discovered through
the IDE controller, the device types and functions are named with `ide_device_`
prefix, rather than something like `ata_device_`. The functions and types
provided are as follows:

- `enum ide_error`: Conglomerate error enum/int for all fallible functions in
  this module.
- `ide_device_t`: An opaque handle referring to one of the drives attached to
  the IDE controller.
- `enum ide_error ata_init(void)`: Initialize the data internal to this driver
  by searching for the first PCI device which appears to be an IDE controller
  and enumerating all the drives attached to it. After calling this, it is
  valid to call `ide_devices_enumerate`.
- `enum ide_error ide_device_read_sectors(ide_device_t, uint16_t numsects, uint32_t lba, uint16_t buf[numsects * 256])`: Given an `ide_device_t`, read a number of sectors (`numsects`) off of it starting at offset `lba`, into memory pointed at by `buf`. Can fail if device faults, the device doesn't exist, if it's an ATAPI device (ATAPI is unimplemented) or if polling the channel fails for any reason (see the `IDE_ERROR_POLL_` entries of the `enum ide_error`.
- `enum ide_error ide_device_write_sectors(ide_device_t, uint16_t numsects, uint32_t lba, uint16_t buf[numsects * 256])`: Given an `ide_device_t`, write a number of sectors (`numsects`) pointed at by `buf` at offset `lba`. Will warn but not fail if the polling operation fails on the disk.
- `struct ide_devicelist ide_devices_enumerate(void)`: Returns a structure which contains a variable number of `ide_device_t`s. It is only valid to call this after calling `ata_init` with return code `IDE_ERROR_OK`.

## clock.c

COMS real time clock driver

## gpu/

Contains drivers for each type of gpu
- Bochs (QEMU default gpu device)
- GOP (Graphics Output Protocol), UEFI framebuffer

## gpu.c

Functions for abstracting over current loaded gpu

## pci.c

PCI (Peripheral Component Interconnect)
- driver to load pci devices (not pcie)

## pit.c

PIT (Programmable Interval Timer)
- sends timer interrupts to the pic
- set pc speaker tones

## ps2.c

PS2 Controller / Keyboard / Mouse driver
- allows keyboard / mouse input

## uart.c

Serial (UART) driver
