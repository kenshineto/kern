### Copyright (c) 2025 Freya Murphy <freya@freyacat.org>

.PHONY: build clean
.SILENT:

AS ?= as
AR ?= ar
CC ?= cc
LD ?= ld
CPP ?= cpp

CPPFLAGS += -Iinclude

CFLAGS += -O2
CFLAGS += -std=c11
CFLAGS += -Wall -Wextra -pedantic
CFLAGS += -fno-pie -fno-stack-protector
CFLAGS += -fno-omit-frame-pointer -ffreestanding
CFLAGS += -fno-builtin
CFLAGS += -D DEBUG -g
CFLAGS += $(CPPFLAGS)

LDFLAGS += -nmagic -nostdlib
LDFLAGS += -z noexecstack

USER=*
LIB=lib
BIN=../bin/user

H_SRC = $(shell find include -type f -name "*.h")
LIBA_SRC = $(shell find $(LIB) -type f -name "*.S")
LIBA_OBJ = $(patsubst %.S,$(BIN)/%.S.o,$(LIBA_SRC))
LIBC_SRC = $(shell find $(LIB) -type f -name "*.c")
LIBC_OBJ = $(patsubst %.c,$(BIN)/%.o,$(LIBC_SRC))
USER_SRC = $(shell find $(USER) -maxdepth 0 -type f -name "*.c")
USER_OBJ = $(patsubst %.c,$(BIN)/%.o,$(USER_SRC))
USER_PROJ = $(patsubst %.o,%,$(USER_OBJ))

build: $(USER_PROJ)

clean:
	rm -fr $(BIN)

$(LIBA_OBJ): $(BIN)/%.S.o : %.S
	mkdir -p $(@D)
	printf "\033[33m  AS  \033[0m%s\n" $<
	$(CPP) $(CPPFLAGS) -o $@.cpp $<
	$(AS) -o $@ $@.cpp

$(LIBC_OBJ): $(BIN)/%.o : %.c
	mkdir -p $(@D)
	printf "\033[34m  CC  \033[0m%s\n" $<
	$(CC) -c $(CFLAGS) -o $@ $<

$(BIN)/libc.a: $(LIBA_OBJ) $(LIBC_OBJ)
	@mkdir -p $(@D)
	$(AR) rcs $@ $(LIBA_OBJ) $(LIBC_OBJ)

$(USER_OBJ): $(BIN)/%.o : %.c
	mkdir -p $(@D)
	printf "\033[34m  CC  \033[0m%s\n" $<
	$(CC) -c $(CFLAGS) -o $@ $<

$(USER_PROJ): % : %.o $(BIN)/libc.a
	mkdir -p $(@D)
	printf "\033[32m  LD  \033[0m%s\n" $@
	$(LD) $(LDFLAGS) -T ../config/user.ld -o $@ $< $(BIN)/libc.a
