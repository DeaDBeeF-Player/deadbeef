/******************************************************************************
*                                                                             *
*  Copyright (C) 1992-1995 Tony Robinson                                      *
*                                                                             *
*  See the file doc/LICENSE.shorten for conditions on distribution and usage  *
*                                                                             *
******************************************************************************/

/*
 * $Id: array.c,v 1.7 2003/08/26 05:34:04 jason Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include "shorten.h"
#include "shn.h"

void *pmalloc(ulong size, shn_file *this_shn) {
  void *ptr;

  ptr = malloc(size);

  if(ptr == NULL)
    shn_error_fatal(this_shn,"Call to malloc(%ld) failed in pmalloc() -\nyour system may be low on memory", size);

  return(ptr);
}

slong **long2d(ulong n0, ulong n1, shn_file *this_shn) {
  slong **array0 = NULL;

  size_t ptr_size = (size_t)n0 * sizeof(slong*);
  size_t data_size = (size_t)n0 * (size_t)n1 * sizeof(slong);
  if (n0 != 0 && data_size / n0 / sizeof(slong) != n1) {
    shn_error_fatal(this_shn, "Integer overflow in long2d(%lu, %lu)", (unsigned long)n0, (unsigned long)n1);
    return NULL;
  }
  size_t total = ptr_size + data_size;
  if (total < ptr_size) {
    shn_error_fatal(this_shn, "Integer overflow in long2d(%lu, %lu)", (unsigned long)n0, (unsigned long)n1);
    return NULL;
  }

  if((array0 = (slong**) pmalloc((ulong) total, this_shn)) != NULL ) {
    slong *array1 = (slong*) (array0 + n0);
    int i;

    for(i = 0; i < n0; i++)
      array0[i] = array1 + i * n1;
  }
  return(array0);
}
