# Comus

Comus is a kernel!

## Dependencies

To build comus, a c11 compiler is required, along with the gnu assembler
and linker.

To build the boot iso for qemu, `grub-mkrescue` is needed from grub.

## Build

Run `make build` to build the kernel.

Run `make clean` to clean the source tree.

Run `make qemu` to build and run in qemu.

Run `make qemu-kvm` to build and run in qemu with kvm enabled.

Run `make qemu-gdb` to build and run in qemu with gdb debugging.

Run `make gdb` to start the gdb debugger.

Run `make fmt` to format the source code.

## Nix

For development on NixOS run `nix develop` in the source directory.

## UEFI

Add `UEFI=1` as an argument to the makefile to build and run in UEFI.

Requires the nix flake.

## Docs

See `docs/`

## License

This project is licensed under the GPLv2
