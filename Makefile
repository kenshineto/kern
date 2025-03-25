
.PHONY: build clean qemu
.SILENT:

QEMU = qemu-system-i386
QEMUOPTS = -drive file=bin/disk.img,index=0,media=disk,format=raw

qemu: bin/disk.img
	$(QEMU) -serial mon:stdio $(QEMUOPTS)


clean:
	rm -fr .zig-cache
	rm -fr bin

build:
	zig build

bin/boot.bin: build
	cd bin && \
		objcopy -S -O binary -j .text boot boot.bin

bin/user.img: build
	cd bin && \
		./mkblob init shell

bin/disk.img: build bin/boot.bin bin/user.img
	cd bin && \
		./BuildImage -d usb -o disk.img -b boot.bin \
		kernel 0x10000 user.img 0x40000

