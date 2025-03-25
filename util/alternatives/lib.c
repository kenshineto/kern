/**
** @file	lib.c
**
** @author	Numerous CSCI-452 classes
**
** @brief	C implementations of common library functions
**
** These are callable from either kernel or user code. Care should be taken
** that user code is linked against these separately from kernel code, to
** ensure separation of the address spaces.
**
** This file exists to pull them all in as a single object file.
*/

#include <common.h>

#include <lib.h>

/*
**********************************************
** MEMORY MANIPULATION FUNCTIONS
**********************************************
*/

#include "common/memset.c"
#include "common/memclr.c"
#include "common/memcpy.c"

/*
**********************************************
** STRING MANIPULATION FUNCTIONS
**********************************************
*/

#include "common/str2int.c"
#include "common/strlen.c"
#include "common/strcmp.c"
#include "common/strcpy.c"
#include "common/strcat.c"
#include "common/pad.c"
#include "common/padstr.c"
#include "common/sprint.c"

/*
**********************************************
** CONVERSION FUNCTIONS
**********************************************
*/

#include "common/cvtuns0.c"
#include "common/cvtuns.c"
#include "common/cvtdec0.c"
#include "common/cvtdec.c"
#include "common/cvthex.c"
#include "common/cvtoct.c"
#include "common/bound.c"
