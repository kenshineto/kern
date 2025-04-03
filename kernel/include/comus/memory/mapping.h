/**
 * @file memory.h
 *
 * @author Freya Murphy
 *
 * Kernel memory declarations
 */

#ifndef _MEMORY_MAPPING_H
#define _MEMORY_MAPPING_H

// paging
#define PAGE_SIZE 4096
#define PAGE_PRESENT 0x1
#define PAGE_WRITE 0x2
#define PAGE_USER 0x4
#define PAGE_HUGE 0x80
#define PAGE_GLOBAL 0x100

#endif
