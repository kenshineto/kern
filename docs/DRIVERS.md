# Drivers

All drivers and their uses

## acpi.c

ACPI (Advanced Configuration and Power Interface)
- allows powering off the system

## ata.c

ATA (Advanced Technology Attachment) / IDE (Integrated Drive Electronics)
- ide/ata disk device driver

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
