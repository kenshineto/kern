/**
 * @file mboot.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * Multiboot functions
 */

#ifndef MBOOT_H_
#define MBOOT_H_

#include <comus/memory.h>

void mboot_load_mmap(volatile void *mboot, struct memory_map *map);

#endif /* mboot.h */
