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

 last mod: $Id: format.h 16825 2010-01-27 04:14:08Z xiphmont $

 ********************************************************************/

#ifndef __FORMAT_H__
#define __FORMAT_H__

#include "audio.h"
#include "transport.h"
#include "ogg123.h"


typedef struct decoder_stats_t {
  double total_time;  /* seconds */
  double current_time;   /* seconds */
  long   instant_bitrate;
  long   avg_bitrate;
} decoder_stats_t;


/* Severity constants */
enum { ERROR, WARNING, INFO };

typedef struct decoder_callbacks_t {
  void (* printf_error) (void *arg, int severity, char *message, ...);
  void (* printf_metadata) (void *arg, int verbosity, char *message, ...);
} decoder_callbacks_t;


struct format_t;

typedef struct decoder_t {
  data_source_t *source;
  audio_format_t request_fmt;
  audio_format_t actual_fmt;
  struct format_t *format;
  decoder_callbacks_t *callbacks;
  void *callback_arg;
  void *private;
} decoder_t;

/* whence constants */
#define DECODER_SEEK_NONE  0
#define DECODER_SEEK_START 1
#define DECODER_SEEK_CUR   2

typedef struct format_t {
  char *name;

  int (* can_decode) (data_source_t *source);
  decoder_t* (* init) (data_source_t *source, ogg123_options_t *ogg123_opts,
		       audio_format_t *audio_fmt,
		       decoder_callbacks_t *callbacks, void *callback_arg);
  int (* read) (decoder_t *decoder, void *ptr, int nbytes, int *eos, 
		audio_format_t *audio_fmt);
  int (* seek) (decoder_t *decoder, double offset, int whence);
  decoder_stats_t* (* statistics) (decoder_t *decoder);
  void (* cleanup) (decoder_t *decoder);
} format_t;

format_t *get_format_by_name (char *name);
format_t *select_format (data_source_t *source);

decoder_stats_t *malloc_decoder_stats (decoder_stats_t *to_copy);

#endif /* __FORMAT_H__ */
