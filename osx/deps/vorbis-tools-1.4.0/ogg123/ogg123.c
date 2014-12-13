/* ogg123.c by Kenneth Arnold <kcarnold-xiph@arnoldnet.net> */
/* Maintained by Stan Seibert <volsung@xiph.org>, Monty <monty@xiph.org> */

/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS BEFORE DISTRIBUTING.                     *
 *                                                                  *
 * THE Ogg123 SOURCE CODE IS (C) COPYRIGHT 2000-2005                *
 * by Stan Seibert <volsung@xiph.org> AND OTHER CONTRIBUTORS        *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 last mod: $Id: ogg123.c 17013 2010-03-24 08:11:06Z xiphmont $

 ********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <locale.h>

#include "audio.h"
#include "buffer.h"
#include "callbacks.h"
#include "cfgfile_options.h"
#include "cmdline_options.h"
#include "format.h"
#include "transport.h"
#include "status.h"
#include "playlist.h"
#include "compat.h"
#include "remote.h"

#include "ogg123.h"
#include "i18n.h"

extern int exit_status; /* from status.c */

void play (char *source_string);

#define PRIMAGIC (2*2*2*2*3*3*3*5*7)
/* take buffer out of the data segment, not the stack */
#define AUDIO_CHUNK_SIZE ((16384 + PRIMAGIC - 1)/ PRIMAGIC * PRIMAGIC)
unsigned char convbuffer[AUDIO_CHUNK_SIZE];
int convsize = AUDIO_CHUNK_SIZE;

ogg123_options_t options;
stat_format_t *stat_format;
buf_t *audio_buffer=NULL;

audio_play_arg_t audio_play_arg;


/* ------------------------- config file options -------------------------- */

/* This macro is used to create some dummy variables to hold default values
   for the options. */
#define INIT(type, value) type type##_##value = value
INIT(int, 1);
INIT(int, 0);

file_option_t file_opts[] = {
  /* found, name, description, type, ptr, default */
  {0, "default_device", N_("default output device"), opt_type_string,
   &options.default_device, NULL},
  {0, "shuffle",        N_("shuffle playlist"),      opt_type_bool,
   &options.shuffle,        &int_0},
  {0, "repeat",         N_("repeat playlist forever"),   opt_type_bool,
   &options.repeat,         &int_0}, 
  {0, NULL,             NULL,                    0,               NULL,                NULL}
};


/* Flags set by the signal handler to control the threads */
signal_request_t sig_request = {0, 0, 0, 0, 0};


/* ------------------------------- signal handler ------------------------- */


void signal_handler (int signo)
{
  struct timeval tv;
  ogg_int64_t now;
  switch (signo) {
  case SIGINT:

    gettimeofday(&tv, 0);

    /* Units of milliseconds (need the cast to force 64 arithmetics) */
    now = (ogg_int64_t) tv.tv_sec * 1000 + tv.tv_usec / 1000;

    if ( (now - sig_request.last_ctrl_c) <= options.delay)
      sig_request.exit = 1;
    else
      sig_request.skipfile = 1;

    sig_request.cancel = 1;
    sig_request.last_ctrl_c = now;
    break;

  case SIGTERM:
    sig_request.exit = 1;
    break;

  case SIGTSTP:
    sig_request.pause = 1;
    /* buffer_Pause (Options.outputOpts.buffer);
       buffer_WaitForPaused (Options.outputOpts.buffer);
       }
       if (Options.outputOpts.devicesOpen == 0) {
       close_audio_devices (Options.outputOpts.devices);
       Options.outputOpts.devicesOpen = 0;
       }
    */
    /* open_audio_devices();
       if (Options.outputOpts.buffer) {
       buffer_Unpause (Options.outputOpts.buffer);
       }
    */
    break;

  case SIGCONT:
    break;  /* Don't need to do anything special to resume */
  }
}

/* -------------------------- util functions ---------------------------- */

void options_init (ogg123_options_t *opts)
{
  opts->verbosity = 2;
  opts->shuffle = 0;
  opts->delay = 500;
  opts->nth = 1;
  opts->ntimes = 1;
  opts->seekoff = 0.0;
  opts->endpos = -1.0; /* Mark as unset */
  opts->seekmode = DECODER_SEEK_NONE;
  opts->buffer_size = 128 * 1024;
  opts->prebuffer = 0.0f;
  opts->input_buffer_size = 64 * 1024;
  opts->input_prebuffer = 50.0f;
  opts->default_device = NULL;

  opts->status_freq = 10.0;
  opts->playlist = NULL;
  opts->remote = 0;
  opts->repeat = 0;

}

double strtotime(char *s)
{
	double time;

	time = strtod(s, &s);

	while (*s == ':')
		time = 60 * time + strtod(s + 1, &s);

	return time;
}

void set_seek_opt(ogg123_options_t *ogg123_opts, char *buf) {

  char *b = buf;

  /* skip spaces */
  while (*b && (*b == ' ')) b++;

  if (*b == '-') {
  /* relative seek back */
    ogg123_opts->seekoff = -1 * strtotime(b+1);
    ogg123_opts->seekmode = DECODER_SEEK_CUR;
  } else
  if (*b == '+') {
  /* relative seek forward */
    ogg123_opts->seekoff = strtotime(b+1);
    ogg123_opts->seekmode = DECODER_SEEK_CUR;
  } else {
  /* absolute seek */
    ogg123_opts->seekoff = strtotime(b);
    ogg123_opts->seekmode = DECODER_SEEK_START;
  }
}

int handle_seek_opt(ogg123_options_t *options, decoder_t *decoder, format_t *format) {

  float pos=decoder->format->statistics(decoder)->current_time;

  /* this functions handles a seek request. It prevents seeking out
     of band, i.e. before the beginning or after the end. Instead,
	 it seeks to the start or near-end resp. */

  if (options->seekmode != DECODER_SEEK_NONE) {

    if (options->seekmode == DECODER_SEEK_START) {
      pos = options->seekoff;
    } else {
      pos += options->seekoff;
    }

    if (pos < 0) {
      pos = 0;
    }

    if (pos > decoder->format->statistics(decoder)->total_time) {
      /* seek to almost the end of the stream */
      pos = decoder->format->statistics(decoder)->total_time - 0.01;
    }

    if (!format->seek(decoder, pos, DECODER_SEEK_START)) {
      status_error(_("Could not skip to %f in audio stream."), options->seekoff);
#if 0
      /* Handle this fatally -- kill the audio thread */
      if (audio_buffer != NULL)
	buffer_thread_kill(audio_buffer);
#endif
    }
  }

  options->seekmode = DECODER_SEEK_NONE;

  return 1;
}

/* This function selects which statistics to display for our
   particular configuration.  This does not have anything to do with
   verbosity, but rather with which stats make sense to display. */
void select_stats (stat_format_t *stats, ogg123_options_t *opts, 
		   data_source_t *source, decoder_t *decoder, 
		   buf_t *audio_buffer)
{
  data_source_stats_t *data_source_stats;

  if (audio_buffer != NULL) {
    /* Turn on output buffer stats */
    stats[8].enabled = 1; /* Fill */
    stats[9].enabled = 1; /* State */
  } else {
    stats[8].enabled = 0;
    stats[9].enabled = 0;
  }

  data_source_stats = source->transport->statistics(source);
  if (data_source_stats->input_buffer_used) {
    /* Turn on input buffer stats */
    stats[6].enabled = 1; /* Fill */
    stats[7].enabled = 1; /* State */
  } else {
    stats[6].enabled = 0;
    stats[7].enabled = 0;
  }
  free(data_source_stats);

  /* Assume we need total time display, and let display_statistics()
     determine at what point it should be turned off during playback */
  stats[2].enabled = 1;  /* Remaining playback time */
  stats[3].enabled = 1;  /* Total playback time */
}


/* Handles printing statistics depending upon whether or not we have 
   buffering going on */
void display_statistics (stat_format_t *stat_format,
			 buf_t *audio_buffer, 
			 data_source_t *source,
			 decoder_t *decoder)
{
  print_statistics_arg_t *pstats_arg;
  buffer_stats_t *buffer_stats;

  pstats_arg = new_print_statistics_arg(stat_format,
					source->transport->statistics(source),
					decoder->format->statistics(decoder));

  if (options.remote) {

    /* Display statistics via the remote interface */
    remote_time(pstats_arg->decoder_statistics->current_time,
                pstats_arg->decoder_statistics->total_time);

  } else {

	/* Disable/Enable statistics as needed */

	if (pstats_arg->decoder_statistics->total_time <
    	pstats_arg->decoder_statistics->current_time) {
      stat_format[2].enabled = 0;  /* Remaining playback time */
      stat_format[3].enabled = 0;  /* Total playback time */
	}

	if (pstats_arg->data_source_statistics->input_buffer_used) {
      stat_format[6].enabled = 1;  /* Input buffer fill % */
      stat_format[7].enabled = 1;  /* Input buffer state  */
	}

	if (audio_buffer) {
      /* Place a status update into the buffer */
      buffer_append_action_at_end(audio_buffer,
				  &print_statistics_action,
				  pstats_arg);

      /* And if we are not playing right now, do an immediate
    	 update just the output buffer */
      buffer_stats = buffer_statistics(audio_buffer);
      if (buffer_stats->paused || buffer_stats->prebuffering) {
    	pstats_arg = new_print_statistics_arg(stat_format,
					      NULL,
					      NULL);
    	print_statistics_action(audio_buffer, pstats_arg);
      }
      free(buffer_stats);

	} else
      print_statistics_action(NULL, pstats_arg);
  }
}


void display_statistics_quick (stat_format_t *stat_format,
			       buf_t *audio_buffer, 
			       data_source_t *source,
			       decoder_t *decoder)
{
  print_statistics_arg_t *pstats_arg;

  pstats_arg = new_print_statistics_arg(stat_format,
					source->transport->statistics(source),
					decoder->format->statistics(decoder));

  if (audio_buffer) {
    print_statistics_action(audio_buffer, pstats_arg);
  } else
    print_statistics_action(NULL, pstats_arg);
}

double current_time (decoder_t *decoder)
{
  decoder_stats_t *stats;
  double ret;

  stats = decoder->format->statistics(decoder);
  ret = stats->current_time;

  free(stats);

  return ret;
}

void print_audio_devices_info(audio_device_t *d)
{
  ao_info *info;

  while (d != NULL) {
    info = ao_driver_info(d->driver_id);

    status_message(2, _("\nAudio Device:   %s"), info->name);
    status_message(3, _("Author:   %s"), info->author);
    status_message(3, _("Comments: %s"), info->comment);
    status_message(2, "");

    d = d->next_device;
  }

}


/* --------------------------- main code -------------------------------- */



int main(int argc, char **argv)
{
  int optind;
  char **playlist_array;
  int items;
  struct stat stat_buf;
  int i;

  setlocale(LC_ALL, "");
  bindtextdomain(PACKAGE, LOCALEDIR);
  textdomain(PACKAGE);

  ao_initialize();
  stat_format = stat_format_create();
  options_init(&options);
  file_options_init(file_opts);

  parse_std_configs(file_opts);
  options.playlist = playlist_create();
  optind = parse_cmdline_options(argc, argv, &options, file_opts);

  audio_play_arg.devices = options.devices;
  audio_play_arg.stat_format = stat_format;

  /* Add remaining arguments to playlist */
  for (i = optind; i < argc; i++) {
    if (stat(argv[i], &stat_buf) == 0) {

      if (S_ISDIR(stat_buf.st_mode)) {
	if (playlist_append_directory(options.playlist, argv[i]) == 0)
	  fprintf(stderr, 
		  _("WARNING: Could not read directory %s.\n"), argv[i]);
      } else {
	playlist_append_file(options.playlist, argv[i]);
      }
    } else /* If we can't stat it, it might be a non-disk source */
      playlist_append_file(options.playlist, argv[i]);


  }


  /* Do we have anything left to play? */
  if (playlist_length(options.playlist) == 0) {
    cmdline_usage();
    exit(1);
  } else {
    playlist_array = playlist_to_array(options.playlist, &items);
    playlist_destroy(options.playlist);
    options.playlist = NULL;
  }

  /* Don't use status_message until after this point! */
  status_init(options.verbosity);

  print_audio_devices_info(options.devices);


  /* Setup buffer */
  if (options.buffer_size > 0) {
    /* Keep sample size alignment for surround sound with up to 10 channels */
    options.buffer_size = (options.buffer_size + PRIMAGIC - 1) / PRIMAGIC * PRIMAGIC;
    audio_buffer = buffer_create(options.buffer_size,
				 options.buffer_size * options.prebuffer / 100,
				 audio_play_callback, &audio_play_arg,
				 AUDIO_CHUNK_SIZE);
    if (audio_buffer == NULL) {
      status_error(_("Error: Could not create audio buffer.\n"));
      exit(1);
    }
  } else
    audio_buffer = NULL;


  /* Setup signal handlers and callbacks */

  signal (SIGINT, signal_handler);
  signal (SIGTSTP, signal_handler);
  signal (SIGCONT, signal_handler);
  signal (SIGTERM, signal_handler);

  if (options.remote) {
    /* run the mainloop for the remote interface */
    remote_mainloop();

  } else {

    do {
      /* Shuffle playlist */
      if (options.shuffle) {
        int i;

        srandom(time(NULL));

        for (i = 0; i < items; i++) {
          int j = i + random() % (items - i);
          char *temp = playlist_array[i];
          playlist_array[i] = playlist_array[j];
          playlist_array[j] = temp;
        }
      }

      /* Play the files/streams */
      i = 0;
      while (i < items && !sig_request.exit) {
        play(playlist_array[i]);
        i++;
      }
    } while (options.repeat);

  }
  playlist_array_destroy(playlist_array, items);
  status_deinit();

  if (audio_buffer != NULL) {
    buffer_destroy (audio_buffer);
    audio_buffer = NULL;
  }

  ao_onexit (options.devices);

  exit (exit_status);
}

void play (char *source_string)
{
  transport_t *transport;
  format_t *format;
  data_source_t *source;
  decoder_t *decoder;

  decoder_callbacks_t decoder_callbacks;
  void *decoder_callbacks_arg;

  /* Preserve between calls so we only open the audio device when we 
     have to */
  static audio_format_t old_audio_fmt = { 0, 0, 0, 0, 0 };
  audio_format_t new_audio_fmt;
  audio_reopen_arg_t *reopen_arg;

  /* Flags and counters galore */
  int eof = 0, eos = 0, ret;
  int nthc = 0, ntimesc = 0;
  int next_status = 0;
  static int status_interval = 0;

  /* Reset all of the signal flags */
  sig_request.cancel   = 0;
  sig_request.skipfile = 0;
  sig_request.exit     = 0;
  sig_request.pause    = 0;

  /* Set preferred audio format (used by decoder) */
  new_audio_fmt.big_endian = ao_is_big_endian();
  new_audio_fmt.signed_sample = 1;
  new_audio_fmt.word_size = 2;

  /* Select appropriate callbacks */
  if (audio_buffer != NULL) {
    decoder_callbacks.printf_error = &decoder_buffered_error_callback;
    decoder_callbacks.printf_metadata = &decoder_buffered_metadata_callback;
    decoder_callbacks_arg = audio_buffer;
  } else {
    decoder_callbacks.printf_error = &decoder_error_callback;
    decoder_callbacks.printf_metadata = &decoder_metadata_callback;
    decoder_callbacks_arg = NULL;
  }

  /* Locate and use transport for this data source */  
  if ( (transport = select_transport(source_string)) == NULL ) {
    status_error(_("No module could be found to read from %s.\n"), source_string);
    return;
  }
  
  if ( (source = transport->open(source_string, &options)) == NULL ) {
    status_error(_("Cannot open %s.\n"), source_string);
    return;
  }

  /* Detect the file format and initialize a decoder */
  if ( (format = select_format(source)) == NULL ) {
    status_error(_("The file format of %s is not supported.\n"), source_string);
    return;
  }

  if ( (decoder = format->init(source, &options, &new_audio_fmt, 
			       &decoder_callbacks,
			       decoder_callbacks_arg)) == NULL ) {

    /* We may have failed because of user command */
    if (!sig_request.cancel)
      status_error(_("Error opening %s using the %s module."
		     "  The file may be corrupted.\n"), source_string,
		   format->name);
    return;
  }

  /* Decide which statistics are valid */
  select_stats(stat_format, &options, source, decoder, audio_buffer);

  /* Start the audio playback thread before we begin sending data */    
  if (audio_buffer != NULL) {

    /* First reset mutexes and other synchronization variables */
    buffer_reset (audio_buffer);
    buffer_thread_start (audio_buffer);
  }

  /* Show which file we are playing */
  decoder_callbacks.printf_metadata(decoder_callbacks_arg, 1,
				    _("Playing: %s"), source_string);

  /* Skip over audio */
  if (options.seekoff > 0.0) {
    /* Note: it may be simpler to handle this condition by just calling:
     *   handle_seek_opt(&options, decoder, format);
     * which was introduced with the remote control interface. However, that
     * function does not call buffer_thread_kill() on error, which is
     * necessary in this situation.
     */
    if (!format->seek(decoder, options.seekoff, DECODER_SEEK_START)) {
      status_error(_("Could not skip %f seconds of audio."), options.seekoff);
      if (audio_buffer != NULL)
	buffer_thread_kill(audio_buffer);
      return;
    }
  }

  /* Main loop:  Iterates over all of the logical bitstreams in the file */
  while (!eof && !sig_request.exit) {

    /* Loop through data within a logical bitstream */
    eos = 0;
    while (!eos && !sig_request.exit) {

      /* Check signals */
      if (sig_request.skipfile) {
	eof = eos = 1;
	break;
      }

	if (options.remote) {
	
		/* run the playloop for the remote interface */
		if (remote_playloop()) {
			/* end song requested */
			eof = eos = 1;
			break;
		}

		/* Skip over audio */
		handle_seek_opt(&options, decoder, format);
	}

	if (sig_request.pause) {
	if (audio_buffer)
	  buffer_thread_pause (audio_buffer);

	kill (getpid(), SIGSTOP); /* We block here until we unpause */
	
	/* Done pausing */
	if (audio_buffer)
	  buffer_thread_unpause (audio_buffer);

	sig_request.pause = 0;
      }


      /* Read another block of audio data */
      ret = format->read(decoder, convbuffer, convsize, &eos, &new_audio_fmt);

      /* Bail if we need to */
      if (ret == 0) {
	eof = eos = 1;
	break;
      } else if (ret < 0) {
	status_error(_("ERROR: Decoding failure.\n"));
	break;
      }

      /* Check to see if the audio format has changed */
      if (!audio_format_equal(&new_audio_fmt, &old_audio_fmt)) {
	old_audio_fmt = new_audio_fmt;
	
	/* Update our status printing interval */
	status_interval = new_audio_fmt.word_size * new_audio_fmt.channels * 
	  new_audio_fmt.rate / options.status_freq;
	next_status = 0;

	reopen_arg = new_audio_reopen_arg(options.devices, &new_audio_fmt);

	if (audio_buffer)
	  buffer_insert_action_at_end(audio_buffer, &audio_reopen_action,
				      reopen_arg);
	else
	  audio_reopen_action(NULL, reopen_arg);
      }


      /* Update statistics display if needed */
      if (next_status <= 0) {
	display_statistics(stat_format, audio_buffer, source, decoder); 
	next_status = status_interval;
      } else
	next_status -= ret;

      if (options.endpos > 0.0 && options.endpos <= current_time(decoder)) {
	eof = eos = 1;
	break;
      }


      /* Write audio data block to output, skipping or repeating chunks
	 as needed */
      do {
	
	if (nthc-- == 0) {
          if (audio_buffer) {
            if (!buffer_submit_data(audio_buffer, convbuffer, ret)) {
              status_error(_("ERROR: buffer write failed.\n"));
              eof = eos = 1;
              break;
            }
          } else
	    audio_play_callback(convbuffer, ret, eos, &audio_play_arg);

	  nthc = options.nth - 1;
	}
	
      } while (!sig_request.exit && !sig_request.skipfile &&
	       ++ntimesc < options.ntimes);

      ntimesc = 0;

    } /* End of data loop */

  } /* End of logical bitstream loop */

  /* Done playing this logical bitstream.  Clean up house. */

  if (audio_buffer) {

    if (!sig_request.exit && !sig_request.skipfile) {
      buffer_mark_eos(audio_buffer);
      buffer_wait_for_empty(audio_buffer);
    }

    buffer_thread_kill(audio_buffer);
  }

  /* Print final stats */
  display_statistics_quick(stat_format, audio_buffer, source, decoder); 

  format->cleanup(decoder);
  transport->close(source);
  status_reset_output_lock();  /* In case we were killed mid-output */

  status_message(1, _("Done."));

  if (sig_request.exit)
    exit (exit_status);
}

