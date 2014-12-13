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

 last mod: $Id: transport.h 16825 2010-01-27 04:14:08Z xiphmont $

 ********************************************************************/

#ifndef __TRANSPORT_H__
#define __TRANSPORT_H__

#include <sys/types.h>
#include <unistd.h>
#include "ogg/os_types.h"
#include "buffer.h"
#include "ogg123.h"

typedef struct data_source_stats_t {
  ogg_int64_t bytes_read;
  int input_buffer_used;  /* flag to show if this data_source uses an
                             input buffer.  Ignore the contents of
                             input_buffer and transfer rate if it is
                             false. */
  long transfer_rate;
  buffer_stats_t input_buffer;
} data_source_stats_t;

struct transport_t;

typedef struct data_source_t {
  char *source_string;
  struct transport_t *transport;
  void *private;
} data_source_t;

typedef struct transport_t {
  char *name;
  int (* can_transport)(char *source_string);
  data_source_t* (* open) (char *source_string, ogg123_options_t *ogg123_opts);
  int (* peek) (data_source_t *source, void *ptr, size_t size, size_t nmemb);
  int (* read) (data_source_t *source, void *ptr, size_t size, size_t nmemb);
  int (* seek) (data_source_t *source, long offset, int whence);
  data_source_stats_t * (* statistics) (data_source_t *source);
  long (* tell) (data_source_t *source);
  void (* close) (data_source_t *source);
} transport_t;

transport_t *get_transport_by_name (char *name);
transport_t *select_transport (char *source);

data_source_stats_t *malloc_data_source_stats (data_source_stats_t *to_copy);

#endif /* __TRANSPORT_H__ */
