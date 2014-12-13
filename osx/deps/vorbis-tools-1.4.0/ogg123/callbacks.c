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

 last mod: $Id: callbacks.c 16825 2010-01-27 04:14:08Z xiphmont $

 ********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>

#include "callbacks.h"
#include "i18n.h"

#define WARNING_VERBOSITY 2
#define INFO_VERBOSITY    3

/* Audio callbacks */

int audio_play_callback (void *ptr, int nbytes, int eos, void *arg)
{
  audio_play_arg_t *play_arg = (audio_play_arg_t *) arg;
  int ret;

  ret = audio_devices_write(play_arg->devices, ptr, nbytes);

  return ret ? nbytes : 0;
}

void audio_reopen_action (buf_t *buf, void *arg)
{
  audio_reopen_arg_t *reopen_arg = (audio_reopen_arg_t *) arg;
  audio_device_t *current;
  ao_sample_format format;

  close_audio_devices (reopen_arg->devices);

  /* Record audio device settings and open the devices */
  format.rate = reopen_arg->format->rate;
  format.channels = reopen_arg->format->channels;
  format.bits = reopen_arg->format->word_size * 8;
  format.byte_format = reopen_arg->format->big_endian ? 
    AO_FMT_BIG : AO_FMT_LITTLE;
  format.matrix = reopen_arg->format->matrix;

  current = reopen_arg->devices;

  while (current != NULL) {
    ao_info *info = ao_driver_info(current->driver_id);

    if (current->filename == NULL)
      current->device = ao_open_live(current->driver_id, &format,
				     current->options);
    else
      current->device = ao_open_file(current->driver_id, current->filename,
				     1 /*overwrite*/, &format, 
				     current->options);

    /* Report errors */
    if (current->device == NULL) {
      switch (errno) {
      case AO_ENODRIVER:
        status_error(_("ERROR: Device not available.\n"));
	break;
      case AO_ENOTLIVE:
	status_error(_("ERROR: %s requires an output filename to be specified with -f.\n"), info->short_name);
	break;
      case AO_EBADOPTION:
	status_error(_("ERROR: Unsupported option value to %s device.\n"),
		     info->short_name);
	break;
      case AO_EOPENDEVICE:
	status_error(_("ERROR: Cannot open device %s.\n"),
		     info->short_name);
	break;
      case AO_EFAIL:
	status_error(_("ERROR: Device %s failure.\n"), info->short_name);
	break;
      case AO_ENOTFILE:
	status_error(_("ERROR: An output file cannot be given for %s device.\n"), info->short_name);
	break;
      case AO_EOPENFILE:
	status_error(_("ERROR: Cannot open file %s for writing.\n"),
		     current->filename);
	break;
      case AO_EFILEEXISTS:
	status_error(_("ERROR: File %s already exists.\n"), current->filename);
	break;
      default:
	status_error(_("ERROR: This error should never happen (%d).  Panic!\n"), errno);
	break;
      }
	 
      /* We cannot recover from any of these errors */
      exit(1);
    }

    current = current->next_device;
  }

  /* Cleanup argument */
  if(reopen_arg->format->matrix)
    free(reopen_arg->format->matrix);
  free(reopen_arg->format);
  free(reopen_arg);
}


audio_reopen_arg_t *new_audio_reopen_arg (audio_device_t *devices,
					  audio_format_t *fmt)
{
  audio_reopen_arg_t *arg;

  if ( (arg = calloc(1,sizeof(audio_reopen_arg_t))) == NULL ) {
    status_error(_("ERROR: Out of memory in new_audio_reopen_arg().\n"));
    exit(1);
  }

  if ( (arg->format = calloc(1,sizeof(audio_format_t))) == NULL ) {
    status_error(_("ERROR: Out of memory in new_audio_reopen_arg().\n"));
    exit(1);
  }

  arg->devices = devices;
  /* Copy format in case fmt is recycled later */
  *arg->format = *fmt;
  if(fmt->matrix)
    arg->format->matrix = strdup(fmt->matrix);

  return arg;
}


/* Statistics callbacks */

void print_statistics_action (buf_t *buf, void *arg)
{
  print_statistics_arg_t *stats_arg = (print_statistics_arg_t *) arg;
  buffer_stats_t *buffer_stats;

  if (buf != NULL)
    buffer_stats = buffer_statistics(buf);
  else
    buffer_stats = NULL;

  status_print_statistics(stats_arg->stat_format,
			  buffer_stats,
			  stats_arg->data_source_statistics,
			  stats_arg->decoder_statistics);

  free(stats_arg->data_source_statistics);
  free(stats_arg->decoder_statistics);
  free(stats_arg);
  free(buffer_stats);
}


print_statistics_arg_t *new_print_statistics_arg (
			       stat_format_t *stat_format,
			       data_source_stats_t *data_source_statistics,
			       decoder_stats_t *decoder_statistics)
{
  print_statistics_arg_t *arg;

  if ( (arg = calloc(1,sizeof(print_statistics_arg_t))) == NULL ) {
    status_error(_("Error: Out of memory in new_print_statistics_arg().\n"));
    exit(1);
  }

  arg->stat_format = stat_format;
  arg->data_source_statistics = data_source_statistics;
  arg->decoder_statistics = decoder_statistics;

  return arg;
}


/* Decoder callbacks */

void decoder_error_callback (void *arg, int severity, char *message, ...)
{
  va_list ap;

  va_start(ap, message);
  switch (severity) {
  case ERROR:
    vstatus_error(message, ap);
    break;
  case WARNING:
    vstatus_message(WARNING_VERBOSITY, message, ap);
    break;
  case INFO:
    vstatus_message(INFO_VERBOSITY, message, ap);
    break;
  }
  va_end(ap);
}


void decoder_metadata_callback (void *arg, int verbosity, char *message, ...)
{
  va_list ap;

  va_start(ap, message);
  vstatus_message(verbosity, message, ap);
  va_end(ap);
}


/* ---------------------------------------------------------------------- */

/* These actions are just wrappers for vstatus_message() and vstatus_error() */

typedef struct status_message_arg_t {
  int verbosity;
  char *message;
} status_message_arg_t;


status_message_arg_t *new_status_message_arg (int verbosity)
{
  status_message_arg_t *arg;

  if ( (arg = calloc(1,sizeof(status_message_arg_t))) == NULL ) {
    status_error(_("ERROR: Out of memory in new_status_message_arg().\n"));
    exit(1);
  }

  arg->verbosity = verbosity;

  return arg;
}


void status_error_action (buf_t *buf, void *arg)
{
  status_message_arg_t *myarg = (status_message_arg_t *) arg;

  status_error("%s", myarg->message);

  free(myarg->message);
  free(myarg);
}


void status_message_action (buf_t *buf, void *arg)
{
  status_message_arg_t *myarg = (status_message_arg_t *) arg;

  status_message(myarg->verbosity, "%s", myarg->message);

  free(myarg->message);
  free(myarg);
}

/* -------------------------------------------------------------- */

void decoder_buffered_error_callback (void *arg, int severity, 
				      char *message, ...)
{
  va_list ap;
  buf_t *buf = (buf_t *)arg;
  status_message_arg_t *sm_arg = new_status_message_arg(0);
  int n, size = 80; /* Guess we need no more than 80 bytes. */


  /* Preformat the string and allocate space for it.  This code taken
     straight from the vsnprintf() man page.  We do this here because
     we might need to reinit ap several times. */
  if ((sm_arg->message = calloc (size,1)) == NULL) {
    status_error(_("Error: Out of memory in decoder_buffered_metadata_callback().\n"));
    exit(1);
  }

  while (1) {
    /* Try to print in the allocated space. */
    va_start(ap, message);
    n = vsnprintf (sm_arg->message, size, message, ap);
    va_end(ap);

    /* If that worked, return the string. */
    if (n > -1 && n < size)
      break;
    /* Else try again with more space. */
    if (n > -1)    /* glibc 2.1 */
      size = n+1; /* precisely what is needed */
    else           /* glibc 2.0 */
      size *= 2;  /* twice the old size */
    if ((sm_arg->message = realloc (sm_arg->message, size)) == NULL) {
      status_error(_("Error: Out of memory in decoder_buffered_metadata_callback().\n"));
      exit(1);
    }
  }


  switch (severity) {
  case ERROR:
    buffer_append_action_at_end(buf, &status_error_action, sm_arg);
    break;
  case WARNING:
    sm_arg->verbosity = WARNING_VERBOSITY;
    buffer_append_action_at_end(buf, &status_message_action, sm_arg);
    break;
  case INFO:
    sm_arg->verbosity = INFO_VERBOSITY;
    buffer_append_action_at_end(buf, &status_message_action, sm_arg);
    break;
  }


}


void decoder_buffered_metadata_callback (void *arg, int verbosity, 
					 char *message, ...)
{
  va_list ap;
  buf_t *buf = (buf_t *)arg;
  status_message_arg_t *sm_arg = new_status_message_arg(0);
  int n, size = 80; /* Guess we need no more than 80 bytes. */


  /* Preformat the string and allocate space for it.  This code taken
     straight from the vsnprintf() man page.  We do this here because
     we might need to reinit ap several times. */
  if ((sm_arg->message = calloc (size,1)) == NULL) {
    status_error(_("ERROR: Out of memory in decoder_buffered_metadata_callback().\n"));
    exit(1);
  }

  while (1) {
    /* Try to print in the allocated space. */
    va_start(ap, message);
    n = vsnprintf (sm_arg->message, size, message, ap);
    va_end(ap);

    /* If that worked, return the string. */
    if (n > -1 && n < size)
      break;
    /* Else try again with more space. */
    if (n > -1)    /* glibc 2.1 */
      size = n+1; /* precisely what is needed */
    else           /* glibc 2.0 */
      size *= 2;  /* twice the old size */
    if ((sm_arg->message = realloc (sm_arg->message, size)) == NULL) {
      status_error(_("ERROR: Out of memory in decoder_buffered_metadata_callback().\n"));
      exit(1);
    }
  }

  sm_arg->verbosity = verbosity;
  buffer_append_action_at_end(buf, &status_message_action, sm_arg);
}
