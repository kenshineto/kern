#
# Makefile fragment for the kernel components of the system.
# 
# Makefile fragment for the kernel component of the system.
#
# THIS IS NOT A COMPLETE Makefile - run GNU make in the top-level
# directory, and this will be pulled in automatically.
#

SUBDIRS += kernel

###################
#  FILES SECTION  #
###################

BOOT_OBJ := $(patsubst %.c, $(BUILDDIR)/%.o, $(BOOT_SRC))

KERN_SRC := kernel/startup.S kernel/isrs.S \
	kernel/cio.c kernel/clock.c kernel/kernel.c kernel/kmem.c \
	kernel/list.c kernel/procs.c kernel/sio.c kernel/support.c \
       	kernel/syscalls.c kernel/user.c kernel/vm.c kernel/vmtables.c

KERN_OBJ := $(patsubst %.c, $(BUILDDIR)/%.o, $(KERN_SRC))
KERN_OBJ := $(patsubst %.S, $(BUILDDIR)/%.o, $(KERN_OBJ))

KCFLAGS := -ggdb
KLDFLAGS := -T kernel/kernel.ld
KLIBS := -lkernel -lcommon

###################
#  RULES SECTION  #
###################

kernel:	$(BUILDDIR)/kernel/kernel.b

$(BUILDDIR)/kernel/%.o: kernel/%.c $(BUILDDIR)/.vars.CFLAGS
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(KCFLAGS) -c -o $@ $<

$(BUILDDIR)/kernel/%.o: kernel/%.S $(BUILDDIR)/.vars.CFLAGS
	@mkdir -p $(@D)
	$(CPP) $(CPPFLAGS) -o $(@D)/$*.s $<
	$(AS) $(ASFLAGS) $(KCFLAGS) -o $@ $(@D)/$*.s -a=$(@D)/$*.lst
	$(RM) -f $(@D)/$*.s

$(BUILDDIR)/kernel/kernel: $(KERN_OBJ)
	@mkdir -p $(@D)
	$(LD) $(KLDFLAGS) $(LDFLAGS) -o $@ $(KERN_OBJ) $(KLIBS)
	$(OBJDUMP) -S $@ > $@.asm
	$(NM) -n $@ > $@.sym
	$(READELF) -a $@ > $@.info

$(BUILDDIR)/kernel/kernel.b: $(BUILDDIR)/kernel/kernel
	$(LD) $(LDFLAGS) -o $(BUILDDIR)/kernel/kernel.b -s \
		--oformat binary -Ttext 0x10000 $(BUILDDIR)/kernel/kernel

# some debugging assist rules
$(BUILDDIR)/kernel/%.i: kernel/%.c $(BUILDDIR)/.vars.CFLAGS
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(KCFLAGS) -E -c $< > $(@D)/$*.i

$(BUILDDIR)/kernel/%.dat: $(BUILDDIR)/kernel/%.o
	@mkdir -p $(@D)
	$(OBJCOPY) -S -O binary -j .data $< $@
	hexdump -C $@ > $(@D)/$*.hex

