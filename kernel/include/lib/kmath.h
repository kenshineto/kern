/**
 * @file kmath.h
 *
 * @author Ian McFarlane <i.mcfarlane2002@gmail.com>
 *
 * Kernel math functions
 */

#ifndef _KMATH_H
#define _KMATH_H

#include <stddef.h>

// min and max both prefer a over b
#define MAX(a, b) ((a) >= (b) ? (a) : (b))
#define MIN(a, b) ((a) <= (b) ? (a) : (b))

#define CLAMP(val, min, max) (MAX((min), MIN((val), (max))))

#endif
