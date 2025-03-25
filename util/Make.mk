#
# Makefile fragment for the utility programs
# 
# THIS IS NOT A COMPLETE Makefile - run GNU make in the top-level
# directory, and this will be pulled in automatically.
#

###################
#  FILES SECTION  #
###################

UTIL_BIN :=	BuildImage Offsets mkblob listblob

###################
#  RULES SECTION  #
###################

# how to make everything
util:	$(UTIL_BIN)

#
# Special rules for creating the utility programs.  These are required
# because we don't want to use the same options as for the standalone
# binaries - we want these to be compiled as "normal" programs.
#

BuildImage:     util/BuildImage.c
	@mkdir -p $(@D)
	$(CC) -std=c99 -ggdb -o BuildImage util/BuildImage.c

mkblob:     util/mkblob.c
	@mkdir -p $(@D)
	$(CC) -std=c99 -ggdb -o mkblob util/mkblob.c

listblob:     util/listblob.c
	@mkdir -p $(@D)
	$(CC) -std=c99 -ggdb -o listblob util/listblob.c

#
# Offsets is compiled using -mx32 to force a 32-bit execution environment
# for a program that runs under a 64-bit operating system.  This ensures
# that pointers and long ints are 32 bits rather than 64 bits, which is
# critical to correctly determining the size of types and byte offsets for
# fields in structs. We also compile with "-fno-builtin" to avoid signature
# clashes between declarations in our system and function declarations
# built into the C compiler.
#
# If compiled with the CPP macro CREATE_HEADER_FILE defined, Offsets
# accepts a command-line argument "-h". This causes it to write its
# output as a standard C header file into a file named "include/offsets.h"
# where it can be included into other source files (e.g., to provide
# sizes of structs in C and assembly, or to provide byte offsets into
# structures for use in assembly code).
#

Offsets:        util/Offsets.c
	@mkdir -p $(@D)
	$(CC) -mx32 -std=c99 $(INCLUDES) -fno-builtin \
		-o Offsets util/Offsets.c
