# Comus

Comus is a kernel!

## Contributors

1. Freya Murphy <freya@freyacat.org>
2. Tristan Miller <trimill@trimill.xyz>
3. Simon Kadesh <simon@kade.sh>
4. Ian McFarlane <i.mcfarlane2002@gmail.com>
5. Galen Sagarin <gsp5307@rit.edu>

## License

This project is licensed under the [GNU General Public License, version 2](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html).

## Source Code

https://g.freya.cat/freya/comus

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

## Docs

See `docs/`

## UEFI

Add `UEFI=1` as an argument to the makefile to build and run in UEFI.

Requires the nix flake.
