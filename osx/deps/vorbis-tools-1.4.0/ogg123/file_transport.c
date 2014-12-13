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

 last mod: $Id: file_transport.c 16825 2010-01-27 04:14:08Z xiphmont $

 ********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "transport.h"
#include "i18n.h"


typedef struct file_private_t {
  FILE *fp;
  data_source_stats_t stats;
  int seekable;
} file_private_t;


transport_t file_transport;  /* Forward declaration */


int file_can_transport (char *source_string)
{
  return 1;  /* The file transport is tested last, so always try it */
}

data_source_t* file_open (char *source_string, ogg123_options_t *ogg123_opts)
{
  data_source_t *source;
  file_private_t *private;

  /* Allocate data source structures */
  source = malloc(sizeof(data_source_t));
  private = malloc(sizeof(file_private_t));

  if (source != NULL && private != NULL) {
    source->source_string = strdup(source_string);
    source->transport = &file_transport;
    source->private = private;

    private->seekable = 1;
    private->stats.transfer_rate = 0;
    private->stats.bytes_read = 0;
    private->stats.input_buffer_used = 0;
  } else {
    fprintf(stderr, _("ERROR: Out of memory.\n"));
    exit(1);
  }

  /* Open file */
  if (strcmp(source_string, "-") == 0) {
    private->fp = stdin;
    private->seekable = 0;
  } else
    private->fp = fopen(source_string, "r");

  if (private->fp == NULL) {
    free(source->source_string);
    free(private);
    free(source);

    return NULL;
  }

  return source;
}


int file_peek (data_source_t *source, void *ptr, size_t size, size_t nmemb)
{
  file_private_t *private = source->private;
  FILE *fp = private->fp;
  int items;
  long start;

  if (!private->seekable) return 0;

  /* Record where we are */
  start = ftell(fp);

  items = fread(ptr, size, nmemb, fp);

  /* Now go back so we maintain the peek() semantics */
  if (fseek(fp, start, SEEK_SET) != 0)
    items = 0; /* Flag error condition since we couldn't seek back to
                  the beginning */

  return items;
}


int file_read (data_source_t *source, void *ptr, size_t size, size_t nmemb)
{
  file_private_t *private = source->private;
  FILE *fp = private->fp;
  int bytes_read;

  bytes_read = fread(ptr, size, nmemb, fp);

  if (bytes_read > 0)
    private->stats.bytes_read += bytes_read;

  return bytes_read;
}


int file_seek (data_source_t *source, long offset, int whence)
{
  file_private_t *private = source->private;
  FILE *fp = private->fp;

  if (!private->seekable) return -1;

  return fseek(fp, offset, whence);  
}


data_source_stats_t * file_statistics (data_source_t *source)
{
  file_private_t *private = source->private;

  return malloc_data_source_stats(&private->stats);
}


long file_tell (data_source_t *source)
{
  file_private_t *private = source->private;
  FILE *fp = private->fp;

  if (!private->seekable) return -1;

  return ftell(fp);
}


void file_close (data_source_t *source)
{
  file_private_t *private = source->private;
  FILE *fp = private->fp;

  fclose(fp);

  free(source->source_string);
  free(source->private);
  free(source);
}


transport_t file_transport = {
  "file",
  &file_can_transport,
  &file_open,
  &file_peek,
  &file_read,
  &file_seek,
  &file_statistics,
  &file_tell,
  &file_close
};
