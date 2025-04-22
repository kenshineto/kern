/**
 * @file paging.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * 64-bit paging functions
 */

#ifndef PAGING_H_
#define PAGING_H_

void paging_init(void);

volatile void *pml4_alloc(void);
void pml4_free(volatile void *pml4);

#endif /* paging.h */
