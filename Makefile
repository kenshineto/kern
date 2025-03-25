#
# Makefile for the 20245 operating system.
#

########################################
# Compilation/assembly definable options
########################################

#
# General options:
#   CLEAR_BSS           include code to clear all BSS space
#   GET_MMAP            get BIOS memory map via int 0x15 0xE820
#   OS_CONFIG           OS-related (vs. just standalone) variations
#   FORCE_INLINING      force "inline" functions to be inlined even if
#                       we aren't compiling with at least -O2
#   MAKE_IDENTITY_MAP   Compile vmtables.c with an "identity" page
#                       map table for the first 4MB of the address space.
#

GEN_OPTIONS = -DCLEAR_BSS -DGET_MMAP -DOS_CONFIG

#
# Debugging options:
#   ANNOUNCE_ENTRY      announce entry and exit from kernel functions
#   RPT_INT_UNEXP       report any 'unexpected' interrupts
#   RPT_INT_MYSTERY     report interrupt 0x27 specifically
#   TRACE_CX            context restore tracing
#   SANITY=n            enable "sanity check" level 'n' (0/1/2/3/4)
#   T_*                 tracing options (see below)
#
# Some modules have their own internal debugging options, described
# in their introductory comments.
#
# Define SANITY as 0 for minimal runtime checking (critical errors only).
# If not defined, SANITY defaults to 9999.
#

DBG_OPTIONS = -DRPT_INT_UNEXP
# DBG_OPTIONS += -DTRACE_CX

#
# T_ options are used to define bits in a "tracing" bitmask, to allow
# checking of individual conditions. The following are defined:
#
#   T_PCB                   PCB alloc/dealloc
#   T_VM                    VM-related tasks
#   T_QUE                   PCB queue manipulation
#   T_SCH, T_DSP            Scheduler and dispatcher
#   T_SCALL, T_SRET         System call entry and exit
#   T_EXIT                  Process exit actions
#   T_FORK, T_EXEC          Fork and exec actions
#   T_INIT                  Module init function tracing
#   T_KM, T_KMFR, T_KMIN    Kmem module tracing
#   T_SIO, T_SIOR, T_SIOW   General SIO module checks
#   T_USER, T_ELF           User module operations
#
# You can add compilation options "on the fly" by using EXTRAS=thing
# on the command line.  For example, to compile with -H (to show the
# hierarchy of #includes):
#
#	make EXTRAS=-H
#

TRACE_OPTIONS = -DT_INIT -DT_USER -DT_ELF -DT_KMIN -DT_VM

USER_OPTIONS = $(GEN_OPTIONS) $(DBG_OPTIONS) $(TRACE_OPTIONS) $(EXTRAS)

##############################################################
# YOU SHOULD NOT NEED TO CHANGE ANYTHING BELOW THIS POINT!!! #
##############################################################

#
# Compilation/assembly control
#

#
# We only want to include from the common header directory
#
INCLUDES = -I./include

#
# All our object code will live here
# 
BUILDDIR = build
LIBDIR = $(BUILDDIR)/lib

#
# Things we need to convert to object form
#
SUBDIRS := 

#
# Compilation/assembly/linking commands and options
#
CPP = cpp
CPPFLAGS = $(USER_OPTIONS) -nostdinc $(INCLUDES)

#
# Compiler/assembler/etc. settings for 32-bit binaries
#
CC = gcc -pipe
CFLAGS = -m32 -fno-pie -std=c99 -fno-stack-protector -fno-builtin -Wall -Wstrict-prototypes -MD $(CPPFLAGS)
# CFLAGS += -O2

AS = as
ASFLAGS = --32

LD = ld
LDFLAGS = -melf_i386 -no-pie -nostdlib -L$(LIBDIR)

AR = ar
#ARFLAGS = rvU
ARFLAGS = rsU

# other programs we use
OBJDUMP = objdump
OBJCOPY = objcopy
NM      = nm
READELF = readelf
PERL    = perl

# delete target files if there is an error, or if make is interrupted
.DELETE_ON_ERROR:

# don't delete intermediate files
.PRECIOUS:	%.o $(BUILDDIR)/boot/%.o $(BUILDDIR)/kernel/%.o \
	$(BUILDDIR)/lib/%.o $(BUILDDIR)/user/%.o

#
# Update $(BUILDDIR)/.vars.X if variable X has changed since the last time
# 'make' was run.
#
# Rules that use variable X should depend on $(BUILDDIR)/.vars.X.  If
# the variable's value has changed, this will update the vars file and
# force a rebuild of the rule that depends on it.
# 

$(BUILDDIR)/.vars.%: FORCE
	echo "$($*)" | cmp -s $@ || echo "$($*)" > $@

.PRECIOUS:	$(BUILDDIR)/.vars.%

.PHONY:	FORCE

#		
# Transformation rules - these ensure that all necessary compilation
# flags are specified
#
# Note use of 'cpp' to convert .S files to temporary .s files: this allows
# use of #include/#define/#ifdef statements. However, the line numbers of
# error messages reflect the .s file rather than the original .S file. 
# (If the .s file already exists before a .S file is assembled, then
# the temporary .s file is not deleted.  This is useful for figuring
# out the line numbers of error messages, but take care not to accidentally
# start fixing things by editing the .s file.)
#
# The .c.X rule produces a .X file which contains the original C source
# code from the file being compiled mixed in with the generated
# assembly language code.  Can be helpful when you need to figure out
# exactly what C statement generated which assembly statements!
#

.SUFFIXES:	.S .b .X .i

.c.X:
	$(CC) $(CFLAGS) -g -c -Wa,-adhln $*.c > $*.X

.c.s:
	$(CC) $(CFLAGS) -S $*.c

#.S.s:
#	$(CPP) $(CPPFLAGS) -o $*.s $*.S

#.S.o:
#	$(CPP) $(CPPFLAGS) -o $*.s $*.S
#	$(AS) $(ASFLAGS) -o $*.o $*.s -a=$*.lst
#	$(RM) -f $*.s

.s.b:
	$(AS) $(ASFLAGS) -o $*.o $*.s -a=$*.lst
	$(LD) $(LDFLAGS) -Ttext 0x0 -s --oformat binary -e begtext -o $*.b $*.o

#.c.o:
#	$(CC) $(CFLAGS) -c $*.c

.c.i:
	$(CC) -E $(CFLAGS) -c $*.c > $*.i

#
# Location of the QEMU binary
#
QEMU = /home/course/csci352/bin/qemu-system-i386

# try to generate a unique GDB port
GDBPORT = $(shell expr `id -u` % 5000 + 25000)

# QEMU's gdb stub command line changed in 0.11
QEMUGDB = $(shell if $(QEMU) -help | grep -q '^-gdb'; \
	then echo "-gdb tcp::$(GDBPORT)"; \
	else echo "-s -p $(GDBPORT)"; fi)

# options for QEMU
#
# run 'make' with -DQEMUEXTRA=xxx to add option 'xxx' when QEMU is run
#
# does not include a '-serial' option, as that may or may not be needed
QEMUOPTS = -drive file=disk.img,index=0,media=disk,format=raw $(QEMUEXTRA)

########################################
# RULES SECTION
########################################

#
# All the individual parts
#

#
# We have a bit of a chicken/egg problem here. When we create the
# user.img file, a new version of include/userids.h is generated
# in build/new_userids.h; this is compared to the existing userids.h
# file, and if they differ, it is copied into include/userids.h.
# This, unfortunately, should trigger a rebuild of anything that 
# includes <userids.h>, but that is all of the user/*.c files along
# with kernel/kernel.c. We could move the user.img creation earlier,
# which would automatically be incorporated into the build of the
# kernel, but it wouldn't automatically trigger recreating the
# userland stuff. We settle for having the build process tell the
# user that a rebuild is required.
#

all:	lib bootstrap kernel userland util user.img disk.img

# Rules etc. for the various sections of the system
include lib/Make.mk
include boot/Make.mk
include user/Make.mk
include kernel/Make.mk
include util/Make.mk

#
# Rules for building the disk image
#

disk.img: $(BUILDDIR)/kernel/kernel.b $(BUILDDIR)/boot/boot user.img BuildImage
	./BuildImage -d usb -o disk.img -b $(BUILDDIR)/boot/boot \
		$(BUILDDIR)/kernel/kernel.b 0x10000 \
		user.img 0x30000

#
# Rules for running with QEMU
#

# how to create the .gdbinit config file if we need it
.gdbinit: util/gdbinit.tmpl
	sed "s/localhost:1234/localhost:$(GDBPORT)/" < $^ > $@

# "ordinary" gdb
gdb:
	gdb -q -n -x .gdbinit

# gdb with the super-fancy Text User Interface
gdb-tui:
	gdb -q -n -x .gdbinit -tui

qemu: disk.img
	$(QEMU) -serial mon:stdio $(QEMUOPTS)

qemu-nox: disk.img
	$(QEMU) -nographic $(QEMUOPTS)

qemu-gdb: disk.img .gdbinit
	@echo "*** Now run 'gdb'." 1>&2
	$(QEMU) -serial mon:stdio $(QEMUOPTS) -S $(QEMUGDB)

qemu-nox-gdb: disk.img .gdbinit
	@echo "*** Now run 'gdb'." 1>&2
	$(QEMU) -nographic $(QEMUOPTS) -S $(QEMUGDB)

#
# Create a printable namelist from the kernel file
#
# kernel.nl:   only global symbols
# kernel.nll:  all symbols
#

kernel.nl: $(BUILDDIR)/kernel/kernel
	nm -Bng $(BUILDDIR)/kernel/kernel.o | pr -w80 -3 > kernel.nl

kernel.nll: $(BUILDDIR)/kernel/kernel
	nm -Bn $(BUILDDIR)/kernel/kernel.o | pr -w80 -3 > kernel.nll

#
# Generate a disassembly
#

kernel.dis: $(BUILDDIR)/kernel/kernel
	objdump -d $(BUILDDIR)/kernel/kernel > kernel.dis

#
# Cleanup etc.
#

clean:
	rm -rf $(BUILDDIR) .gdbinit *.nl *.nll *.lst *.i *.X *.dis

realclean:	clean
	rm -f LOG *.img $(UTIL_BIN)

#
# Automatically generate dependencies for header files included
# from C source files.
#
$(BUILDDIR)/.deps: $(foreach dir, $(SUBDIRS), $(wildcard $(BUILDDIR)/$(dir)/*.d))
	@mkdir -p $(@D)
	$(PERL) util/mergedep.pl $@ $^

-include $(BUILDDIR)/.deps

.PHONY:	all clean realclean
