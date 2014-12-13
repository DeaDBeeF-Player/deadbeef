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
 
 last mod: $Id: audio.h 16825 2010-01-27 04:14:08Z xiphmont $
 
********************************************************************/

/* ogg123's audio playback functions */

#ifndef __AUDIO_H__
#define __AUDIO_H__

#include <ao/ao.h>


typedef struct audio_format_t {
  int big_endian;
  int word_size;
  int signed_sample;
  int rate;
  int channels;
  char *matrix; 
} audio_format_t;


/* For facilitating output to multiple devices */
typedef struct audio_device_t {
  int driver_id;
  ao_device *device;
  ao_option *options;
  char *filename;
  struct audio_device_t *next_device;
} audio_device_t;


int audio_format_equal (audio_format_t *a, audio_format_t *b);
audio_device_t *append_audio_device(audio_device_t *devices_list,
				     int driver_id,
				     ao_option *options, char *filename);
void audio_devices_print_info(audio_device_t *d);
int audio_devices_write(audio_device_t *d, void *ptr, int nbytes);
int add_ao_option(ao_option **op_h, const char *optstring);
void close_audio_devices (audio_device_t *devices);
void free_audio_devices (audio_device_t *devices);
void ao_onexit (void *arg);

#endif /* __AUDIO_H__ */
