#
# Makefile fragment for the library components of the system.
# 
# THIS IS NOT A COMPLETE Makefile - run GNU make in the top-level
# directory, and this will be pulled in automatically.
#

SUBDIRS += lib

###################
#  FILES SECTION  #
###################

#
# library file lists
#

# "common" library functions, used by kernel and users
CLIB_SRC := lib/bound.c lib/cvtdec.c lib/cvtdec0.c \
	lib/cvthex.c lib/cvtoct.c lib/cvtuns.c \
	lib/cvtuns0.c lib/memclr.c lib/memcpy.c \
	lib/memset.c lib/pad.c lib/padstr.c \
	lib/sprint.c lib/str2int.c lib/strcat.c \
	lib/strcmp.c lib/strcpy.c lib/strlen.c

# user-only library functions
ULIB_SRC := lib/ulibc.c lib/ulibs.S lib/entry.S

# kernel-only library functions
KLIB_SRC := lib/klibc.c

# lists of object files
CLIB_OBJ:= $(patsubst lib/%.c, $(BUILDDIR)/lib/%.o, $(CLIB_SRC))

ULIB_OBJ:= $(patsubst lib/%.c, $(BUILDDIR)/lib/%.o, $(ULIB_SRC))
ULIB_OBJ:= $(patsubst lib/%.S, $(BUILDDIR)/lib/%.o, $(ULIB_OBJ))

KLIB_OBJ := $(patsubst lib/%.c, $(BUILDDIR)/lib/%.o, $(KLIB_SRC))
KLIB_OBJ := $(patsubst lib/%.S, $(BUILDDIR)/lib/%.o, $(KLIB_OBJ))

# library file names
CLIB_NAME := libcommon.a
ULIB_NAME := libuser.a
KLIB_NAME := libkernel.a

###################
#  RULES SECTION  #
###################

# how to make everything
lib:	$(BUILDDIR)/lib/$(CLIB_NAME) \
	$(BUILDDIR)/lib/$(KLIB_NAME) \
	$(BUILDDIR)/lib/$(ULIB_NAME)

$(BUILDDIR)/lib/%.o:	lib/%.c $(BUILDDIR)/.vars.CFLAGS
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR)/lib/%.o:	lib/%.S $(BUILDDIR)/.vars.CFLAGS
	@mkdir -p $(@D)
	$(CPP) $(CPPFLAGS) -o $(@D)/$*.s $<
	$(AS) $(ASFLAGS) -o $@ $(@D)/$*.s -a=$(@D)/$*.lst
	$(RM) -f $(@D)/$*.s
	$(NM) -n $@ > $(@D)/$*.sym

$(BUILDDIR)/lib/$(CLIB_NAME):	$(CLIB_OBJ)
	$(AR) $(ARFLAGS) $@ $(CLIB_OBJ)

$(BUILDDIR)/lib/$(ULIB_NAME):	$(ULIB_OBJ)
	$(AR) $(ARFLAGS) $@ $(ULIB_OBJ)

$(BUILDDIR)/lib/$(KLIB_NAME):	$(KLIB_OBJ)
	$(AR) $(ARFLAGS) $@ $(KLIB_OBJ)
