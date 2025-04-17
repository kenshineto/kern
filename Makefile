### Copyright (c) 2025 Freya Murphy <freya@freyacat.org>

.PHONY: build fmt clean qemu
.SILENT:

AS ?= as
CC ?= cc
LD ?= ld
CPP ?= cpp

CPPFLAGS += -Ikernel/include

CFLAGS += -O0
CFLAGS += -std=c11
CFLAGS += -Wall -Wextra -pedantic
CFLAGS += -fno-pie -fno-stack-protector
CFLAGS += -fno-omit-frame-pointer -ffreestanding
CFLAGS += -fno-builtin
CFLAGS += -D DEBUG -g
CFLAGS += $(CPPFLAGS)

LDFLAGS += -nmagic --no-warn-rwx-segments -nostdlib -E

SRC=kernel
BIN=bin
KERNEL=kernel.bin
ISO=os.iso

H_SRC = $(shell find $(SRC) -type f -name "*.h")
A_SRC = $(shell find $(SRC) -type f -name "*.S")
A_OBJ = $(patsubst %.S,$(BIN)/%.S.o,$(A_SRC))
C_SRC = $(shell find $(SRC) -type f -name "*.c")
C_OBJ = $(patsubst %.c,$(BIN)/%.o,$(C_SRC))

UNAME := $(shell uname)

QEMU = qemu-system-x86_64
QEMUOPTS = -cdrom $(BIN)/$(ISO) \
		   -no-reboot \
		   -serial mon:stdio \
		   -m 4G \
		   -name kern

qemu: $(BIN)/$(ISO)
	$(QEMU) $(QEMUOPTS)

qemu-kvm: $(BIN)/$(ISO)
	$(QEMU) -cpu host --enable-kvm $(QEMUOPTS)

qemu-nox: $(BIN)/$(ISO)
	$(QEMU) $(QEMUOPTS) -nographic

qemu-gdb: $(BIN)/$(ISO)
	$(QEMU) $(QEMUOPTS) -S -gdb tcp::1337

qemu-gdb-nox: $(BIN)/$(ISO)
	$(QEMU) $(QEMUOPTS) -nographic -S -gdb tcp::1337

gdb:
	gdb -q -n -x config/gdbinit

clean:
	rm -fr $(BIN)

build: $(BIN)/$(ISO)

$(A_OBJ): $(BIN)/%.S.o : %.S
	mkdir -p $(@D)
	printf "\033[33m  AS  \033[0m%s\n" $<
	$(CPP) $(CPPFLAGS) -o $@.cpp $<
	$(AS) -o $@ $@.cpp

$(C_OBJ): $(BIN)/%.o : %.c
	mkdir -p $(@D)
	printf "\033[34m  CC  \033[0m%s\n" $<
	$(CC) -c $(CFLAGS) -o $@ $<

$(BIN)/$(KERNEL): $(C_OBJ) $(A_OBJ)
	mkdir -p $(@D)
	printf "\033[32m  LD  \033[0m%s\n" $@
	$(LD) $(LDFLAGS) -T config/kernel.ld -o $(BIN)/$(KERNEL) $(A_OBJ) $(C_OBJ)

$(BIN)/$(ISO): $(BIN)/$(KERNEL)
	mkdir -p $(BIN)/iso/boot/grub
	cp config/grub.cfg $(BIN)/iso/boot/grub
	cp $(BIN)/$(KERNEL) $(BIN)/iso/boot
	grub-mkrescue -o $(BIN)/$(ISO) bin/iso 2>/dev/null

fmt:
	clang-format -i $(shell find -type f -name "*.[ch]" -and -not -path "./kernel/old/*")
	sed -i 's/[ \t]*$$//' $(shell find -type f -name "*.[chS]" -and -not -path "./kernel/old/*")
