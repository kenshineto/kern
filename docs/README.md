Comus is a 64-bit paging kernel.

Created for CSCI.452.01

## Arch

amd64

## Bootloader

Multiboot (Legacy / UEFI)

## Docstrings

Kernel function docstrings MUST be located in the headers.

Docstrings for global functions MAY be in the source c files.

## Startup

1. Multiboot loads kernel into either `_start` or `_start_efi`
  -  Kernel identity maps during legacy boot (`_start`)
2. Kernel loads GDT, and far jobs into `main`
3. `main` loads the modules `cpu`, `mboot`, `memory`, `drivers`, `fs`, `pcb` in order.
  - See MODULES.md
4. Kernel loads init process (`bin/init`)
5. Init loads user programs (sendoff!)
