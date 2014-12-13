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

 last mod: $Id: status.c 16825 2010-01-27 04:14:08Z xiphmont $

 ********************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "status.h"
#include "i18n.h"

char temp_buffer[200];
int last_line_len = 0;
int max_verbosity = 0;
int exit_status = EXIT_SUCCESS;

pthread_mutex_t output_lock = PTHREAD_MUTEX_INITIALIZER;


/* ------------------- Private functions ------------------ */

void unlock_output_lock (void *arg)
{
  pthread_mutex_unlock(&output_lock);
}


void write_buffer_state_string (char *dest, buffer_stats_t *buf_stats)
{
  char *cur = dest;
  char *comma = ", ";
  char *sep = "(";

  if (buf_stats->prebuffering) {
    cur += sprintf (cur, _("%sPrebuf to %.1f%%"), sep, 
		    100.0f * buf_stats->prebuffer_fill);
    sep = comma;
  }
  if (buf_stats->paused) {
    cur += sprintf (cur, _("%sPaused"), sep);
    sep = comma;
  }
  if (buf_stats->eos) {
    cur += sprintf (cur, _("%sEOS"), sep);
    sep = comma;
  }
  if (cur != dest)
    cur += sprintf (cur, ")");
  else
    *cur = '\0';
}


/* Write a min:sec.msec style string to dest corresponding to time.
   The time parameter is in seconds.  Returns the number of characters
   written */
int write_time_string (char *dest, double time)
{
  long min = (long) time / (long) 60;
  double sec = time - 60.0f * min;

  return sprintf (dest, "%02li:%05.2f", min, sec);
}


void clear_line (int len)
{
  fputc('\r', stderr);

  while (len > 0) {
    fputc (' ', stderr);
    len--;
  }

  fputc ('\r', stderr);
}


int sprintf_clear_line(int len, char *buf)
{
  int i = 0;

  buf[i] = '\r';
  i++;

  while (len > 0) {
    buf[i] = ' ';
    len--;
    i++;
  }

  buf[i] = '\r';
  i++;

  /* Null terminate just in case */
  buf[i] = '\0';

  return i;
}

int print_statistics_line (stat_format_t stats[])
{
  int len = 0;
  char *str = temp_buffer;

  if (max_verbosity == 0)
    return 0;

  /* Put the clear line text into the same string buffer so that the
     line is cleared and redrawn all at once.  This reduces
     flickering.  Don't count characters used to clear line in len */
  str += sprintf_clear_line(last_line_len, str); 

  while (stats->formatstr != NULL) {
    
    if (stats->verbosity > max_verbosity || !stats->enabled) {
      stats++;
      continue;
    }

    if (len != 0)
      len += sprintf(str+len, " ");

    switch (stats->type) {
    case stat_noarg:
      len += sprintf(str+len, stats->formatstr);
      break;
    case stat_intarg:
      len += sprintf(str+len, stats->formatstr, stats->arg.intarg);
      break;
    case stat_stringarg:
      len += sprintf(str+len, stats->formatstr, stats->arg.stringarg);
      break;
    case stat_floatarg:
      len += sprintf(str+len, stats->formatstr, stats->arg.floatarg);
      break;
    case stat_doublearg:
      len += sprintf(str+len, stats->formatstr, stats->arg.doublearg);
      break;
    }

    stats++;
  }

  len += sprintf(str+len, "\r");

  fprintf(stderr, "%s", temp_buffer);

  return len;
}


void vstatus_print_nolock (const char *fmt, va_list ap)
{
  if (last_line_len != 0)
    fputc ('\n', stderr);

  vfprintf (stderr, fmt, ap);

  fputc ('\n', stderr);

  last_line_len = 0;
}


/* ------------------- Public interface -------------------- */

#define TIME_STR_SIZE 20
#define STATE_STR_SIZE 25
#define NUM_STATS 10

stat_format_t *stat_format_create ()
{
  stat_format_t *stats;
  stat_format_t *cur;

  stats = calloc(NUM_STATS + 1, sizeof(stat_format_t));  /* One extra for end flag */
  if (stats == NULL) {
    fprintf(stderr, _("Memory allocation error in stats_init()\n"));
    exit(1);
  }

  cur = stats + 0; /* currently playing file / stream */
  cur->verbosity = 3; 
  cur->enabled = 0;
  cur->formatstr = _("File: %s"); 
  cur->type = stat_stringarg;
  
  cur = stats + 1; /* current playback time (preformatted) */
  cur->verbosity = 1;
  cur->enabled = 1;
  cur->formatstr = _("Time: %s"); 
  cur->type = stat_stringarg;
  cur->arg.stringarg = calloc(TIME_STR_SIZE, sizeof(char));

  if (cur->arg.stringarg == NULL) {
    fprintf(stderr, _("Memory allocation error in stats_init()\n"));
    exit(1);
  }  
  write_time_string(cur->arg.stringarg, 0.0);


  cur = stats + 2; /* remaining playback time (preformatted) */
  cur->verbosity = 1;
  cur->enabled = 1;
  cur->formatstr = "[%s]";
  cur->type = stat_stringarg;
  cur->arg.stringarg = calloc(TIME_STR_SIZE, sizeof(char));

  if (cur->arg.stringarg == NULL) {
    fprintf(stderr, _("Memory allocation error in stats_init()\n"));
    exit(1);
  }
  write_time_string(cur->arg.stringarg, 0.0);


  cur = stats + 3; /* total playback time (preformatted) */
  cur->verbosity = 1;
  cur->enabled = 1;
  cur->formatstr = _("of %s");
  cur->type = stat_stringarg;
  cur->arg.stringarg = calloc(TIME_STR_SIZE, sizeof(char));

  if (cur->arg.stringarg == NULL) {
    fprintf(stderr, _("Memory allocation error in stats_init()\n"));
    exit(1);
  }
  write_time_string(cur->arg.stringarg, 0.0);


  cur = stats + 4; /* instantaneous bitrate */
  cur->verbosity = 2;
  cur->enabled = 1;
  cur->formatstr = " (%5.1f kbps)";
  cur->type = stat_doublearg;

  cur = stats + 5; /* average bitrate (not yet implemented) */
  cur->verbosity = 2;
  cur->enabled = 0;
  cur->formatstr = _("Avg bitrate: %5.1f");
  cur->type = stat_doublearg;

  cur = stats + 6; /* input buffer fill % */
  cur->verbosity = 2;
  cur->enabled = 0;
  cur->formatstr = _(" Input Buffer %5.1f%%");
  cur->type = stat_doublearg;

  cur = stats + 7; /* input buffer status */
  cur->verbosity = 2;
  cur->enabled = 0;
  cur->formatstr = "%s";
  cur->type = stat_stringarg;
  cur->arg.stringarg = calloc(STATE_STR_SIZE, sizeof(char));

  if (cur->arg.stringarg == NULL) {
    fprintf(stderr, _("Memory allocation error in stats_init()\n"));
    exit(1);
  }


  cur = stats + 8; /* output buffer fill % */
  cur->verbosity = 2;
  cur->enabled = 0;
  cur->formatstr = _(" Output Buffer %5.1f%%"); 
  cur->type = stat_doublearg;

  cur = stats + 9; /* output buffer status */
  cur->verbosity = 1;
  cur->enabled = 0;
  cur->formatstr = "%s";
  cur->type = stat_stringarg;
  cur->arg.stringarg = calloc(STATE_STR_SIZE, sizeof(char));

  if (cur->arg.stringarg == NULL) {
    fprintf(stderr, _("Memory allocation error in stats_init()\n"));
    exit(1);
  }


  cur = stats + 10; /* End flag */
  cur->formatstr = NULL;

  return stats;
}


void stat_format_cleanup (stat_format_t *stats)
{
  free(stats[1].arg.stringarg);
  free(stats[2].arg.stringarg);
  free(stats[3].arg.stringarg);
  free(stats[7].arg.stringarg);
  free(stats[9].arg.stringarg);
  free(stats);
}


void status_init (int verbosity)
{
#if defined(HAVE_FCNTL) && defined(HAVE_UNISTD_H)
  fcntl (STDERR_FILENO, F_SETFL, fcntl(STDERR_FILENO, F_GETFL) | O_NONBLOCK);
#endif

  max_verbosity = verbosity;
}

void status_deinit ()
{
#if defined(HAVE_FCNTL) && defined(HAVE_UNISTD_H)
  fcntl (STDERR_FILENO, F_SETFL, fcntl(STDERR_FILENO, F_GETFL) & ~O_NONBLOCK);
#endif
}

void status_reset_output_lock ()
{
  pthread_mutex_unlock(&output_lock);
}


void status_clear_line ()
{
  pthread_cleanup_push(unlock_output_lock, NULL);

  pthread_mutex_lock(&output_lock);
  clear_line(last_line_len);
  pthread_mutex_unlock(&output_lock);

  pthread_cleanup_pop(0);
}

void status_print_statistics (stat_format_t *stats,
			      buffer_stats_t *audio_statistics,
			      data_source_stats_t *transport_statistics,
			      decoder_stats_t *decoder_statistics)
{
  pthread_cleanup_push(unlock_output_lock, NULL);

  /* Updating statistics is not critical.  If another thread is
     already doing output, we skip it. */
  if (pthread_mutex_trylock(&output_lock) == 0) {

    if (decoder_statistics != NULL) {
      /* Current playback time */
      write_time_string(stats[1].arg.stringarg,
			decoder_statistics->current_time);
	
      /* Remaining playback time */
      write_time_string(stats[2].arg.stringarg,
			decoder_statistics->total_time - 
			decoder_statistics->current_time);

      /* Total playback time */
      write_time_string(stats[3].arg.stringarg,
			decoder_statistics->total_time);

      /* Instantaneous bitrate */
      stats[4].arg.doublearg = decoder_statistics->instant_bitrate / 1000.0f;

      /* Instantaneous bitrate */
      stats[5].arg.doublearg = decoder_statistics->avg_bitrate / 1000.0f;
    }


    if (transport_statistics != NULL && 
	transport_statistics->input_buffer_used) {

      /* Input buffer fill % */
      stats[6].arg.doublearg = transport_statistics->input_buffer.fill;

      /* Input buffer state */
      write_buffer_state_string(stats[7].arg.stringarg,
				&transport_statistics->input_buffer);
    }


    if (audio_statistics != NULL) {

      /* Output buffer fill % */
      stats[8].arg.doublearg = audio_statistics->fill;

      /* Output buffer state */
      write_buffer_state_string(stats[9].arg.stringarg, audio_statistics);
    }

    last_line_len = print_statistics_line(stats);

    pthread_mutex_unlock(&output_lock);
  }

  pthread_cleanup_pop(0);
}


void status_message (int verbosity, const char *fmt, ...)
{
  va_list ap;

  if (verbosity > max_verbosity)
    return;

  pthread_cleanup_push(unlock_output_lock, NULL);

  pthread_mutex_lock(&output_lock);

  clear_line(last_line_len);

  va_start (ap, fmt);
  vstatus_print_nolock(fmt, ap);
  va_end (ap);

  pthread_mutex_unlock(&output_lock);

  pthread_cleanup_pop(0);
}


void vstatus_message (int verbosity, const char *fmt, va_list ap)
{
  if (verbosity > max_verbosity)
    return;

  pthread_cleanup_push(unlock_output_lock, NULL);

  pthread_mutex_lock(&output_lock);

  clear_line(last_line_len);
  vstatus_print_nolock(fmt, ap);

  pthread_mutex_unlock(&output_lock);

  pthread_cleanup_pop(0);
}


void status_error (const char *fmt, ...)
{
  va_list ap;

  pthread_cleanup_push(unlock_output_lock, NULL);

  pthread_mutex_lock(&output_lock);
  va_start (ap, fmt);
  clear_line(last_line_len);
  vstatus_print_nolock (fmt, ap);
  va_end (ap);
  pthread_mutex_unlock(&output_lock);

  pthread_cleanup_pop(0);

  exit_status = EXIT_FAILURE;
}


void vstatus_error (const char *fmt, va_list ap)
{
  pthread_cleanup_push(unlock_output_lock, NULL);

  pthread_mutex_lock(&output_lock);
  clear_line(last_line_len);
  vstatus_print_nolock (fmt, ap);
  pthread_mutex_unlock(&output_lock);

  pthread_cleanup_pop(0);

  exit_status = EXIT_FAILURE;
}
