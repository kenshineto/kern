### Copyright (c) 2025 Freya Murphy <freya@freyacat.org>

.PHONY: build fmt clean qemu
.SILENT:

UNAME := $(shell uname)

QEMU = qemu-system-x86_64
QEMUOPTS = -drive file=bin/disk.img,index=0,media=disk,format=raw \
		   -no-reboot \
		   -serial mon:stdio \
		   -m 4G \
		   -name kern

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
	sed -i 's/[ \t]*$$//' $(shell find -type f -name "*.[chS]" -or -name "*.ld")

bin/boot.bin: build
	cd bin && \
		objcopy -S -O binary -j .text boot boot.bin

bin/kernel.bin: build
	cd bin && \
		objcopy -S -O binary kernel kernel.bin

bin/user.img: build
	cd bin && \
		./mkblob init idle prog* shell

bin/disk.img: bin/kernel.bin bin/boot.bin
	cd bin && \
		./BuildImage -d usb -o disk.img -b boot.bin \
		kernel.bin 0x10000

