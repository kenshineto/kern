/**
** @file list.c
**
** @author Warren R. Carithers
**
** @brief	Support for a basic linked list data type.
**
** This module provides a very basic linked list data structure.
** A list can contain anything that has a pointer field in the first
** four bytes; these routines assume those bytes contain a pointer to
** the following entry in the list, whatever that may be.
*/

#define KERNEL_SRC

#include <common.h>

#include <list.h>

/*
** FUNCTIONS
*/

/**
** Name:    list_add
**
** Add the supplied data to the beginning of the specified list.
**
** @param[in,out] list  The address of a list_t variable
** @param[in] data      The data to prepend to the list
*/
void list_add( list_t *list, void *data ) {

	// sanity checks
	assert1( list != NULL );
	assert1( data != NULL );

	list_t *tmp = (list_t *)data;
	tmp->next = list->next;
	list->next = tmp;
}

/**
** Name:    list_remove
**
** Remove the first entry from a linked list.
**
** @param[in,out] list  The address of a list_t variable
**
** @return a pointer to the removed data, or NULL if the list was empty
*/
void *list_remove( list_t *list ) {

	assert1( list != NULL );

	list_t *data = list->next;
	if( data != NULL ) {
		list->next = data->next;
		data->next = NULL;
	}

	return (void *)data;
}

