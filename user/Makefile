### Copyright (c) 2025 Freya Murphy <freya@freyacat.org>

include ../config.mk

.PHONY: build clean
.SILENT:

CFLAGS += -fPIC -mcmodel=large

USER=*
LIB=lib
BIN=bin
LINKER=../config/user.ld
CONFIG=../config.mk
TAR=initrd.tar

H_SRC = $(shell find include -type f -name "*.h")
LIBA_SRC = $(shell find $(LIB) -type f -name "*.S")
LIBA_OBJ = $(patsubst %.S,$(BIN)/%.S.o,$(LIBA_SRC))
LIBC_SRC = $(shell find $(LIB) -type f -name "*.c")
LIBC_OBJ = $(patsubst %.c,$(BIN)/%.o,$(LIBC_SRC))
USER_SRC = $(shell find $(USER) -maxdepth 0 -type f -name "*.c")
USER_OBJ = $(patsubst %.c,$(BIN)/%.o,$(USER_SRC))
USER_PROG = $(patsubst %.o,%,$(USER_OBJ))

build: $(BIN)/$(TAR)

clean:
	rm -fr $(BIN)

$(BIN)/$(TAR): $(USER_PROG)
	printf "\033[35m  TAR \033[0m%s\n" $@
	mkdir -p $(BIN)/initrd/bin
	cp $(USER_PROG) $(BIN)/initrd/bin
	tar cHf ustar $(BIN)/$(TAR) -C $(BIN)/initrd bin
	rm -fr $(BIN)/initrd

$(LIBA_OBJ): $(BIN)/%.S.o : %.S $(H_SRC) $(CONFIG)
	mkdir -p $(@D)
	printf "\033[33m  AS  \033[0m%s\n" $<
	$(CPP) $(CPPFLAGS) -o $@.cpp $<
	$(AS) -o $@ $@.cpp

$(LIBC_OBJ): $(BIN)/%.o : %.c $(H_SRC) $(CONFIG)
	mkdir -p $(@D)
	printf "\033[34m  CC  \033[0m%s\n" $<
	$(CC) -c $(CFLAGS) -o $@ $<

$(BIN)/libc.a: $(LIBA_OBJ) $(LIBC_OBJ)
	@mkdir -p $(@D)
	printf "\033[35m  AR  \033[0m%s\n" $@
	$(AR) rcs $@ $(LIBA_OBJ) $(LIBC_OBJ)

$(USER_OBJ): $(BIN)/%.o : %.c $(H_SRC) $(CONFIG)
	mkdir -p $(@D)
	printf "\033[34m  CC  \033[0m%s\n" $<
	$(CC) -c $(CFLAGS) -o $@ $<

$(USER_PROG): % : $(BIN)/libc.a %.o $(LINKER)
	mkdir -p $(@D)
	printf "\033[32m  LD  \033[0m%s\n" $@
	$(LD) $(LDFLAGS) -T $(LINKER) -o $@ $@.o $(BIN)/libc.a
