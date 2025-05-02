### Copyright (c) 2025 Freya Murphy <freya@freyacat.org>

.PHONY: build clean fmt qemu qemu-kvm qemu-gdb gdb
.SILENT:

BIN=bin
ISO=os.iso

QEMU ?= qemu-system-x86_64
GRUB ?= grub-mkrescue

QEMUOPTS += -cdrom $(BIN)/$(ISO) \
		    -no-reboot \
		    -drive format=raw,file=user/bin/forkman \
		    -serial mon:stdio \
		    -m 4G \
		    -name kern

ifdef UEFI
QEMU = qemu-system-x86_64-uefi
GRUB = grub-mkrescue-uefi
endif

ifndef DISPLAY
QEMUOPTS += -nographic
endif

qemu: $(BIN)/$(ISO)
	$(QEMU) $(QEMUOPTS)

qemu-kvm: $(BIN)/$(ISO)
	$(QEMU) $(QEMUOPTS) -cpu host --enable-kvm

qemu-gdb: $(BIN)/$(ISO)
	$(QEMU) $(QEMUOPTS) -S -gdb tcp::1337

gdb:
	gdb -q -n -x config/gdbinit

build:
	make -s -C kernel
	make -s -C user

clean:
	rm -fr $(BIN)
	make -s -C kernel clean
	make -s -C user clean

fmt:
	clang-format -i $(shell find -type f -name "*.[ch]" -and -not -path "./kernel/old/*")
	sed -i 's/[ \t]*$$//' $(shell find -type f -name "*.[chS]" -and -not -path "./kernel/old/*")

$(BIN)/$(ISO): build config/grub.cfg
	printf "\033[35m  ISO \033[0m%s\n" $@
	mkdir -p $(BIN)/iso/boot/grub
	cp config/grub.cfg $(BIN)/iso/boot/grub
	cp kernel/bin/kernel $(BIN)/iso/boot
	$(GRUB) -o $(BIN)/$(ISO) bin/iso 2>/dev/null

