/**
 * @file paging.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * 64-bit paging functions
 */

#ifndef PAGING_H_
#define PAGING_H_

#include <stdbool.h>

void paging_init(void);

volatile void *pgdir_alloc(void);
volatile void *pgdir_clone(volatile const void *pdir, bool cow);
void pgdir_free(volatile void *addr);

#endif /* paging.h */
