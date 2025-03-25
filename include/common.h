/**
** @file	common.h
**
** @author	Warren R. Carithers
**
** @brief	Common definitions for the baseline system.
**
** This header file pulls in the standard header information needed
** by all parts of the system. It is purely for our convenience.
*/

#ifndef COMMON_H_
#define COMMON_H_

// everything needs these; they also pull in
// the kernel- or user-level defs and lib headers
#include <types.h>
#include <params.h>
#include <defs.h>
#include <lib.h>

#ifdef KERNEL_SRC

// only kernel code needs these headers
#include <common.h>
#include <cio.h>
#include <debug.h>

#endif  /* KERNEL_SRC */

#endif
