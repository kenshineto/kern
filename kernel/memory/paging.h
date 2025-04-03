/**
 * @file paging.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * 64-bit paging functions
 */

#ifndef PAGING_H_
#define PAGING_H_

#define F_PRESENT 0x001
#define F_WRITEABLE 0x002
#define F_UNPRIVILEGED 0x004
#define F_WRITETHROUGH 0x008
#define F_CACHEDISABLE 0x010
#define F_ACCESSED 0x020
#define F_DIRTY 0x040
#define F_MEGABYTE 0x080
#define F_GLOBAL 0x100

void paging_init(void);

#endif /* paging.h */
