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

 last mod: $Id: audio.c 17006 2010-03-24 01:12:20Z xiphmont $

 ********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "audio.h"


int audio_format_equal (audio_format_t *a, audio_format_t *b)
{
  return 
    a->big_endian    == b->big_endian    &&
    a->word_size     == b->word_size     &&
    a->signed_sample == b->signed_sample &&
    a->rate          == b->rate          &&
    a->channels      == b->channels      &&
    ((a->matrix==NULL && b->matrix==NULL) ||
     !strcmp(a->matrix,b->matrix));
}

audio_device_t *append_audio_device(audio_device_t *devices_list,
				     int driver_id,
				     ao_option *options, char *filename)
{
  audio_device_t *head = devices_list;

  if (devices_list != NULL) {
    while (devices_list->next_device != NULL)
      devices_list = devices_list->next_device;
    devices_list = devices_list->next_device =
      malloc(sizeof(audio_device_t));
  } else {
    head = devices_list = (audio_device_t *) malloc(sizeof(audio_device_t));
  }
  devices_list->driver_id = driver_id;
  devices_list->options = options;
  devices_list->filename = filename;
  devices_list->device = NULL;
  devices_list->next_device = NULL;

  return devices_list;
}


int audio_devices_write(audio_device_t *d, void *ptr, int nbytes)
{

  while (d != NULL) {
    if (ao_play(d->device, ptr, nbytes) == 0)
      return 0; /* error occurred */
    d = d->next_device;
  }

  return 1;
}

int add_ao_option(ao_option **op_h, const char *optstring)
{
  char *key, *value;
  int result;

  key = strdup(optstring);
  if (key == NULL)
    return 0;

  value = strchr(key, ':');
  if (value) {
    /* split by replacing the separator with a null */
    *value++ = '\0';
  }

  result = ao_append_option(op_h, key, value);
  free(key);

  return (result);
}

void close_audio_devices (audio_device_t *devices)
{
  audio_device_t *current = devices;

  while (current != NULL) {
    if (current->device)
      ao_close(current->device);
    current->device = NULL;
    current = current->next_device;
  }
}

void free_audio_devices (audio_device_t *devices)
{
  audio_device_t *current;

  while (devices != NULL) {
    current = devices->next_device;
    free (devices);
    devices = current;
  }
}

void ao_onexit (void *arg)
{
  audio_device_t *devices = (audio_device_t *) arg;

  close_audio_devices (devices);
  free_audio_devices (devices);

  ao_shutdown();
}
