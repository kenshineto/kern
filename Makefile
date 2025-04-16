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

qemu-kvm: bin/os.iso
	$(QEMU) -cpu host --enable-kvm $(QEMUOPTS)

qemu-nox: bin/os.iso
	$(QEMU) $(QEMUOPTS) -nographic

qemu-gdb: bin/os.iso
	$(QEMU) $(QEMUOPTS) -S -gdb tcp::1337

qemu-gdb-nox: bin/os.iso
	$(QEMU) $(QEMUOPTS) -nographic -S -gdb tcp::1337

gdb:
	gdb -q -n -x config/gdbinit

clean:
	rm -fr .zig-cache
	rm -fr bin

build:
	zig build

bin/os.iso: build
	mkdir -p bin/iso/boot/grub
	cp config/grub.cfg bin/iso/boot/grub
	cp bin/kernel bin/iso/boot
	grub-mkrescue -o bin/os.iso bin/iso 2>/dev/null

fmt:
	clang-format -i $(shell find -type f -name "*.[ch]" -and -not -path "./kernel/old/*")
	sed -i 's/[ \t]*$$//' $(shell find -type f -name "*.[chS]" -and -not -path "./kernel/old/*")
