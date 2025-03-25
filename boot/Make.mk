#
# Makefile fragment for the bootstrap component of the system.
# 
# THIS IS NOT A COMPLETE Makefile - run GNU make in the top-level
# directory, and this will be pulled in automatically.
#

SUBDIRS += boot

###################
#  FILES SECTION  #
###################

BOOT_SRC := boot/boot.S

BOOT_OBJ := $(BUILDDIR)/boot/boot.o

# BLDFLAGS := -Ttext 0x7c00 -s --oformat binary -e bootentry
# BLDFLAGS := -Ttext 0 -s --oformat binary -e bootentry
# BLDFLAGS := -N -Ttext 0x7c00 -s -e bootentry
BLDFLAGS := -N -Ttext 0 -s -e bootentry

###################
#  RULES SECTION  #
###################

bootstrap: $(BUILDDIR)/boot/boot

$(BUILDDIR)/boot/%.o:	boot/%.c $(BUILDDIR)/.vars.CFLAGS
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR)/boot/%.o:	boot/%.S $(BUILDDIR)/.vars.CFLAGS
	@mkdir -p $(@D)
	$(CPP) $(CPPFLAGS) -o $(@D)/$*.s $<
	$(AS) $(ASFLAGS) -o $@ $(@D)/$*.s -a=$(@D)/$*.lst
	$(RM) -f $(@D)/$*.s
	$(NM) -n $@ > $(@D)/$*.sym

$(BUILDDIR)/boot/boot: $(BOOT_OBJ)
	@mkdir -p $(@D)
	$(LD) $(LDFLAGS) $(BLDFLAGS) -o $@.out $^
	$(OBJCOPY) -S -O binary -j .text $@.out $@
