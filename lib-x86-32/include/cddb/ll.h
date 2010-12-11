/*
    $Id: ll.h,v 1.1 2005/05/29 08:24:04 airborne Exp $

    Copyright (C) 2005 Kris Verbeeck <airborne@advalvas.be>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the
    Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA  02111-1307, USA.
*/

#ifndef LL_H
#define LL_H 1

#ifdef __cplusplus
    extern "C" {
#endif


/* --- type definitions */


/**
 * Linked list element.
 */
typedef struct elem_s elem_t;

/**
 * Linked list.
 */
typedef struct list_s list_t;

/**
 * Callback prototype for destroying element data.
 */
typedef void elem_destroy_cb(void *data);


/* --- construction / destruction */


/**
 * Creates a new linked list.
 *
 * @param cb The callback used to destroy the element data or NULL if
 *           no further action is required.
 * @return The linked list structure or NULL if memory allocation failed.
 */
list_t *list_new(elem_destroy_cb *cb);

/**
 * Free all resources associated with the given linked list.  Embedded
 * data will be freed by use of the callback registered at list
 * creation.
 *
 * @param list The linked list.
 */
void list_destroy(list_t *list);

/**
 * Remove all elements from the list without destroying the list
 * itself.  Embedded data will be freed by use of the callback
 * registered at list creation.
 *
 * @param list The linked list.
 */
void list_flush(list_t *list);


/* --- list elements --- */

/**
 * Retrieves the data associated with a list element.
 *
 * @param elem The list element.
 * @return The data associated with the element or NULL if the element
 *         was invalid.
 */
void *element_data(elem_t *elem);


/* --- getters & setters --- */


/**
 * Append a new element to the end of the list.
 *
 * @param list The linked list.
 * @param data The data to append to the list.
 * @return The list element that was appended or NULL if memory
 *         allocation fails or the list is invalid.
 */
elem_t *list_append(list_t *list, void *data);

/**
 * Returns the number of elements in the list.
 *
 * @param list The linked list.
 * @return The number of elements.
 */
int list_size(list_t *list);

/**
 * Returns the list element at the specified index or NULL if the
 * index is invalid.
 *
 * @param list The linked list.
 * @param idx The element index (first = 0).
 * @return The element or NULL if not found.
 */
elem_t *list_get(list_t *list, int idx);

/**
 * Returns the first list element.
 *
 * @param list The linked list.
 * @return The first element or NULL if the list is empty.
 */
elem_t *list_first(list_t *list);

/**
 * Returns the next list element.  Before using this function you
 * should call list_first to initialize the iterator.
 *
 * @param list The linked list.
 * @return The next element or NULL if not found.
 */
elem_t *list_next(list_t *list);


/* --- iteration */


#ifdef __cplusplus
    }
#endif

#endif /* LL_H */
