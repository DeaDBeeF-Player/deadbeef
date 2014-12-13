/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS BEFORE DISTRIBUTING.                     *
 *                                                                  *
 * THE Ogg123 SOURCE CODE IS (C) COPYRIGHT 2000-2001                *
 * by Stan Seibert <volsung@xiph.org> AND OTHER CONTRIBUTORS        *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 last mod: $Id: transport.c 16825 2010-01-27 04:14:08Z xiphmont $

 ********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>

#include "transport.h"
#include "i18n.h"

extern transport_t file_transport;
#ifdef HAVE_CURL
extern transport_t http_transport;
#endif

transport_t *transports[] = {
#ifdef HAVE_CURL
  &http_transport,
#endif
  &file_transport,
  NULL
};


transport_t *get_transport_by_name (char *name)
{
  int i = 0;

  while (transports[i] != NULL && strcmp(name, transports[i]->name) != 0)
    i++;

  return transports[i];
}


transport_t *select_transport (char *source)
{
  int i = 0;

  while (transports[i] != NULL && !transports[i]->can_transport(source))
    i++;

  return transports[i];
}


data_source_stats_t *malloc_data_source_stats (data_source_stats_t *to_copy)
{
  data_source_stats_t *new_stats;

  new_stats = malloc(sizeof(data_source_stats_t));

  if (new_stats == NULL) {
    fprintf(stderr, _("ERROR: Could not allocate memory in malloc_data_source_stats()\n"));
    exit(1);
  }

  *new_stats = *to_copy;  /* Copy the data */

  return new_stats;
}
