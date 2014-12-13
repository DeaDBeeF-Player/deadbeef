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

 last mod: $Id: format.c 16825 2010-01-27 04:14:08Z xiphmont $

 ********************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>

#include "transport.h"
#include "format.h"
#include "i18n.h"

extern format_t oggvorbis_format;
extern format_t speex_format;

#ifdef HAVE_LIBFLAC
extern format_t flac_format;
extern format_t oggflac_format;
#endif

#ifdef HAVE_LIBSPEEX
extern format_t speex_format;
#endif

format_t *formats[] = { 
#ifdef HAVE_LIBFLAC
			&flac_format,
			&oggflac_format,
#endif
#ifdef HAVE_LIBSPEEX
			&speex_format,
#endif
			&oggvorbis_format, 
			NULL };


format_t *get_format_by_name (char *name)
{
  int i = 0;

  while (formats[i] != NULL && strcmp(name, formats[i]->name) != 0)
    i++;

  return formats[i];
}


format_t *select_format (data_source_t *source)
{
  int i = 0;

  while (formats[i] != NULL && !formats[i]->can_decode(source))
    i++;

  return formats[i];
}


decoder_stats_t *malloc_decoder_stats (decoder_stats_t *to_copy)
{
  decoder_stats_t *new_stats;

  new_stats = malloc(sizeof(decoder_stats_t));

  if (new_stats == NULL) {
    fprintf(stderr, _("ERROR: Could not allocate memory in malloc_decoder_stats()\n"));
    exit(1);
  }

  *new_stats = *to_copy;  /* Copy the data */

  return new_stats;
}
