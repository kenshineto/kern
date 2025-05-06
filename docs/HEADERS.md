
# Headers

## comus

All kernel headers for the `comus` kernel. Each of these headers map directly
to c files in the kernel source tree besides the following.

### asm.h

Contains inline functions for calling x64/x86 assembly

### error.h

Kernel error codes
- Success (0)
- Failure (1)
- All other codes are specific failures

### keycodes.h

Kernel keycodes used in input.h
- ps2 driver uses these keycodes when mapping its scancodes

### limits.h

Maximum limists across multiple components of the kernel.
- max open files
- max filesystem disks
- max number of process
- etc...

## efi.h

Standard UEFI header. Read UEFI specification online for more information.

## elf.h

Standard ELF header. Read ELF specification online for more information.

## lib

Used for kernel c library code stored in `kernel/lib`
