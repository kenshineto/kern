### Copyright (c) 2025 Freya Murphy <freya@freyacat.org>

include ../config.mk

.PHONY: build fmt clean qemu
.SILENT:

SRC=*
BIN=bin
KERNEL=kernel
LINKER=../config/kernel.ld
CONFIG=../config.mk

H_SRC = $(shell find $(SRC) -type f -name "*.h")
A_SRC = $(shell find $(SRC) -type f -name "*.S")
A_OBJ = $(patsubst %.S,$(BIN)/%.S.o,$(A_SRC))
C_SRC = $(shell find $(SRC) -type f -name "*.c")
C_OBJ = $(patsubst %.c,$(BIN)/%.o,$(C_SRC))

build: $(BIN)/$(KERNEL)

clean:
	rm -fr $(BIN)

$(A_OBJ): $(BIN)/%.S.o : %.S $(H_SRC) $(CONFIG)
	mkdir -p $(@D)
	printf "\033[33m  AS  \033[0m%s\n" $<
	$(CPP) $(CPPFLAGS) -o $@.cpp $<
	$(AS) -o $@ $@.cpp

$(C_OBJ): $(BIN)/%.o : %.c $(H_SRC) $(CONFIG)
	mkdir -p $(@D)
	printf "\033[34m  CC  \033[0m%s\n" $<
	$(CC) -c $(CFLAGS) -o $@ $<

$(BIN)/$(KERNEL): $(A_OBJ) $(C_OBJ) $(LINKER)
	mkdir -p $(@D)
	printf "\033[32m  LD  \033[0m%s\n" $@
	$(LD) $(LDFLAGS) -T $(LINKER) -o $(BIN)/$(KERNEL) $(A_OBJ) $(C_OBJ)
