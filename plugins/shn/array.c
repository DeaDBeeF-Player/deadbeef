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

  if((array0 = (slong**) pmalloc((ulong) (n0 * sizeof(slong*) +
					 n0 * n1 * sizeof(slong)),this_shn)) != NULL ) {
    slong *array1 = (slong*) (array0 + n0);
    int i;

    for(i = 0; i < n0; i++)
      array0[i] = array1 + i * n1;
  }
  return(array0);
}
