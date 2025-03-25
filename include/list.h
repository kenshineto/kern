/**
** @file	list.h
**
** @author	Warren R. Carithers
**
** @brief	Support for a basic linked list data type.
**
** This module provides a very basic linked list data structure.
** A list can contain anything that has a pointer field in the first
** four bytes; these routines assume those bytes contain a pointer to
** the following entry in the list, whatever that may be.
*/

#ifndef LIST_H_
#define LIST_H_

#define KERNEL_SRC

// standard types etc.
#include <common.h>

/*
** General (C and/or assembly) definitions
*/

#ifndef ASM_SRC

/*
** Start of C-only definitions
*/

/*
** Data types
*/

// The list structure
typedef struct list_s {
	struct list_s *next;	// link to the successor
} list_t;

/*
** Prototypes
*/

/**
** Name:    list_add
**
** Add the supplied data to the beginning of the specified list.
**
** @param[in,out] list  The address of a list_t variable
** @param[in] data      The data to prepend to the list
*/
void list_add( list_t *list, void *data );

/**
** Name:    list_remove
**
** Remove the first entry from a linked list.
**
** @param[in,out] list  The address of a list_t variable
**
** @return a pointer to the removed data, or NULL if the list was empty
*/
void *list_remove( list_t *list );

#endif /* !ASM_SRC */

#endif
