
AS ?= as
AR ?= ar
CC ?= cc
LD ?= ld
CPP ?= cpp

CPPFLAGS += -Iinclude

CFLAGS += -O2
CFLAGS += -std=c11
CFLAGS += -Wall -Wextra -pedantic
CFLAGS += -no-pie -fno-stack-protector
CFLAGS += -fno-omit-frame-pointer -ffreestanding
CFLAGS += -nostdlib -fno-builtin -mno-red-zone
CFLAGS += -D DEBUG -g
CFLAGS += $(CPPFLAGS)

LDFLAGS += -nmagic -nostdlib
LDFLAGS += -z noexecstack
