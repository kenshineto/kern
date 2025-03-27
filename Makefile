### Copyright (c) 2025 Freya Murphy <freya@freyacat.org>

.PHONY: build fmt clean qemu
.SILENT:

UNAME := $(shell uname)

QEMU = qemu-system-i386
QEMUOPTS = -drive file=bin/disk.img,index=0,media=disk,format=raw \
		   -no-reboot -d cpu_reset \
		   -serial mon:stdio \
		   -m 4G \
		   -name kern

ifeq ($(UNAME), Linux)
QEMUOPTS += -enable-kvm -display sdl
endif

qemu: bin/disk.img
	$(QEMU) $(QEMUOPTS)

qemu-gdb: bin/disk.img
	$(QEMU) $(QEMUOPTS) -S -gdb tcp::1337

gdb:
	gdb -q -n -x util/gdbinit

clean:
	rm -fr .zig-cache
	rm -fr bin

build:
	zig build

fmt:
	clang-format -i $(shell find -type f -name "*.[ch]")

bin/boot.bin: build
	cd bin && \
		objcopy -S -O binary -j .text boot boot.bin

bin/user.img: build
	cd bin && \
		./mkblob init shell

bin/disk.img: build bin/boot.bin bin/user.img
	cd bin && \
		./BuildImage -d usb -o disk.img -b boot.bin \
		kernel 0x10000 user.img 0x30000

