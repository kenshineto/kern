### Copyright (c) 2025 Freya Murphy <freya@freyacat.org>

.PHONY: build fmt clean qemu
.SILENT:

UNAME := $(shell uname)

QEMU = qemu-system-x86_64
QEMUOPTS = -cdrom bin/os.iso \
		   -no-reboot \
		   -serial mon:stdio \
		   -m 4G \
		   -name kern

qemu: bin/os.iso
	$(QEMU) $(QEMUOPTS)

qemu-gdb: bin/os.iso
	$(QEMU) $(QEMUOPTS) -S -gdb tcp::1337

gdb:
	gdb -q -n -x util/gdbinit

clean:
	rm -fr .zig-cache
	rm -fr bin

build:
	zig build

bin/os.iso: build
	mkdir -p bin/iso/boot/grub
	cp grub.cfg bin/iso/boot/grub
	cp bin/kernel bin/iso/boot
	grub-mkrescue -o bin/os.iso bin/iso 2>/dev/null

fmt:
	clang-format -i $(shell find -type f -name "*.[ch]")
	sed -i 's/[ \t]*$$//' $(shell find -type f -name "*.[chS]" -or -name "*.ld")
