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

volatile void *paging_alloc(void);
void paging_free(volatile void *addr);

#endif /* paging.h */
