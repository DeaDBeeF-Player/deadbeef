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

 last mod: $Id: callbacks.h 16825 2010-01-27 04:14:08Z xiphmont $

 ********************************************************************/

#ifndef __CALLBACKS_H__
#define __CALLBACKS_H__

#include "audio.h"
#include "buffer.h"
#include "status.h"
#include "format.h"
#include "transport.h"


/* Audio callbacks */

typedef struct audio_play_arg_t {
  stat_format_t *stat_format;
  audio_device_t *devices;
} audio_play_arg_t;

typedef struct audio_reopen_arg_t {
  audio_device_t *devices;
  audio_format_t *format;
} audio_reopen_arg_t;

int audio_play_callback (void *ptr, int nbytes, int eos, void *arg);
void audio_reopen_action (buf_t *buf, void *arg);

audio_reopen_arg_t *new_audio_reopen_arg (audio_device_t *devices,
					  audio_format_t *fmt);


/* Statisitics callbacks */

typedef struct print_statistics_arg_t {
  stat_format_t *stat_format;
  data_source_stats_t *data_source_statistics;
  decoder_stats_t *decoder_statistics;
} print_statistics_arg_t;

void print_statistics_action (buf_t *buf, void *arg);
print_statistics_arg_t *new_print_statistics_arg (
			       stat_format_t *stat_format,
			       data_source_stats_t *data_source_statistics,
			       decoder_stats_t *decoder_statistics);


/* Decoder callbacks */
void decoder_error_callback (void *arg, int severity, char *message, ...);
void decoder_metadata_callback (void *arg, int verbosity, char *message, ...);

void decoder_buffered_error_callback (void *arg, int severity, 
				      char *message, ...);
void decoder_buffered_metadata_callback (void *arg, int verbosity, 
					 char *message, ...);


#endif /* __CALLBACKS_H__ */
