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

 last mod: $Id: cmdline_options.c 17009 2010-03-24 04:29:17Z xiphmont $

 ********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ao/ao.h>

#include "getopt.h"
#include "cmdline_options.h"
#include "status.h"
#include "i18n.h"

#define MIN_INPUT_BUFFER_SIZE 8

extern double strtotime(char *s);
extern void set_seek_opt(ogg123_options_t *ogg123_opts, char *buf);

struct option long_options[] = {
  /* GNU standard options */
    {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, 'V'},
    /* ogg123-specific options */
    {"buffer", required_argument, 0, 'b'},
    {"config", optional_argument, 0, 'c'},
    {"device", required_argument, 0, 'd'},
    {"file", required_argument, 0, 'f'},
    {"skip", required_argument, 0, 'k'},
    {"end", required_argument, 0, 'K'},
    {"delay", required_argument, 0, 'l'},
    {"device-option", required_argument, 0, 'o'},
    {"prebuffer", required_argument, 0, 'p'},
    {"quiet", no_argument, 0, 'q'},
    {"remote", no_argument, 0, 'R'},
    {"verbose", no_argument, 0, 'v'},
    {"nth", required_argument, 0, 'x'},
    {"ntimes", required_argument, 0, 'y'},
    {"shuffle", no_argument, 0, 'z'},
    {"random", no_argument, 0, 'Z'},
    {"list", required_argument, 0, '@'},
    {"audio-buffer", required_argument, 0, 0},
    {"repeat", no_argument, 0, 'r'},
    {0, 0, 0, 0}
};

int parse_cmdline_options (int argc, char **argv,
			   ogg123_options_t *ogg123_opts,
			   file_option_t    *file_opts)
{
  int option_index = 1;
  ao_option *temp_options = NULL;
  ao_option ** current_options = &temp_options;
  ao_info *info;
  int temp_driver_id = -1;
  audio_device_t *current;
  int ret;

  while (-1 != (ret = getopt_long(argc, argv, "b:c::d:f:hl:k:K:o:p:qrRvVx:y:zZ@:",
				  long_options, &option_index))) {

      switch (ret) {
      case 0:
	if(!strcmp(long_options[option_index].name, "audio-buffer")) {
	  ogg123_opts->buffer_size = 1024 * atoi(optarg);
	} else {
	  status_error(_("Internal error parsing command line options.\n"));
	  exit(1);
	}
	break;
      case 'b':
	ogg123_opts->input_buffer_size = atoi(optarg) * 1024;
	if (ogg123_opts->input_buffer_size < MIN_INPUT_BUFFER_SIZE * 1024) {
	  status_error(_("Input buffer size smaller than minimum size of %dkB."),
		       MIN_INPUT_BUFFER_SIZE);
	  ogg123_opts->input_buffer_size = MIN_INPUT_BUFFER_SIZE * 1024;
	}
	break;
	
      case 'c':
	if (optarg) {
	  char *tmp = strdup (optarg);
	  parse_code_t pcode = parse_line(file_opts, tmp);

	  if (pcode != parse_ok)
	    status_error(_("=== Error \"%s\" while parsing config option from command line.\n"
			 "=== Option was: %s\n"),
			 parse_error_string(pcode), optarg);
	  free (tmp);
	}
	else {
	  /* not using the status interface here */
	  fprintf (stdout, _("Available options:\n"));
	  file_options_describe(file_opts, stdout);
	  exit (0);
	}
	break;
	
      case 'd':
	temp_driver_id = ao_driver_id(optarg);
	if (temp_driver_id < 0) {
	    status_error(_("=== No such device %s.\n"), optarg);
	    exit(1);
	}

	current = append_audio_device(ogg123_opts->devices,
				      temp_driver_id, 
				      NULL, NULL);
	if(ogg123_opts->devices == NULL)
	  ogg123_opts->devices = current;
	current_options = &current->options;
	break;
	
      case 'f':
	if (temp_driver_id >= 0) {

	  info = ao_driver_info(temp_driver_id);
	  if (info->type == AO_TYPE_FILE) {
	    free(current->filename);
	    current->filename = strdup(optarg);
	  } else {
	    status_error(_("=== Driver %s is not a file output driver.\n"),
			 info->short_name);
	    exit(1);
	  }
	} else {
	  status_error(_("=== Cannot specify output file without specifying a driver.\n"));
	  exit (1);
	}
	break;

	case 'k':
 	  set_seek_opt(ogg123_opts, optarg);
	  break;
	  
	case 'K':
	  ogg123_opts->endpos = strtotime(optarg);
	  break;
	  
	case 'l':
	  ogg123_opts->delay = atoi(optarg);
	  break;
	  
	case 'o':
	  if (optarg && !add_ao_option(current_options, optarg)) {
	    status_error(_("=== Incorrect option format: %s.\n"), optarg);
	    exit(1);
	  }
	  break;

	case 'h':
	  cmdline_usage();
	  exit(0);
	  break;

	case 'p':
	  ogg123_opts->input_prebuffer = atof (optarg);
	  if (ogg123_opts->input_prebuffer < 0.0f || 
	      ogg123_opts->input_prebuffer > 100.0f) {

	    status_error (_("--- Prebuffer value invalid. Range is 0-100.\n"));
	    ogg123_opts->input_prebuffer = 
	      ogg123_opts->input_prebuffer < 0.0f ? 0.0f : 100.0f;
	  }
	  break;

      case 'q':
	ogg123_opts->verbosity = 0;
	break;
	
      case 'r':
        ogg123_opts->repeat = 1;
	break;

      case 'R':
	ogg123_opts->remote = 1;
	ogg123_opts->verbosity = 0;
	break;

      case 'v':
	ogg123_opts->verbosity++;
	break;
	
      case 'V':
	status_error(_("ogg123 from %s %s"), PACKAGE, VERSION);
	exit(0);
	break;

      case 'x':
	ogg123_opts->nth = atoi(optarg);
	if (ogg123_opts->nth == 0) {
	  status_error(_("--- Cannot play every 0th chunk!\n"));
	  ogg123_opts->nth = 1;
	}
	break;
	  
      case 'y':
	ogg123_opts->ntimes = atoi(optarg);
	if (ogg123_opts->ntimes == 0) {
	  status_error(_("--- Cannot play every chunk 0 times.\n"
		 "--- To do a test decode, use the null output driver.\n"));
	  ogg123_opts->ntimes = 1;
	}
	break;
	
      case 'z':
	ogg123_opts->shuffle = 1;
	break;

      case 'Z':
        ogg123_opts->repeat = ogg123_opts->shuffle = 1;
	break;

      case '@':
	if (playlist_append_from_file(ogg123_opts->playlist, optarg) == 0)
	  status_error(_("--- Cannot open playlist file %s.  Skipped.\n"),
		       optarg);
	break;
		
      case '?':
	break;
	
      default:
	cmdline_usage();
	exit(1);
      }
  }

  /* Sanity check bad option combinations */
  if (ogg123_opts->endpos > 0.0 &&
      ogg123_opts->seekoff > ogg123_opts->endpos) {
    status_error(_("=== Option conflict: End time is before start time.\n"));
    exit(1);
  }


  /* Add last device to device list or use the default device */
  if (temp_driver_id < 0) {

      /* First try config file setting */
      if (ogg123_opts->default_device) {
	  temp_driver_id = ao_driver_id(ogg123_opts->default_device);

	  if (temp_driver_id < 0)
	    status_error(_("--- Driver %s specified in configuration file invalid.\n"),
			 ogg123_opts->default_device);
      }

      /* Then try libao autodetect */
      if (temp_driver_id < 0)
	temp_driver_id = ao_default_driver_id();

      /* Finally, give up */
      if (temp_driver_id < 0) {
	status_error(_("=== Could not load default driver and no driver specified in config file. Exiting.\n"));
	exit(1);
      }

      ogg123_opts->devices = append_audio_device(ogg123_opts->devices,
					     temp_driver_id,
					     temp_options, 
					     NULL);
    }

  /* if verbosity has been altered, add options to drivers... */
  {
    audio_device_t *head = ogg123_opts->devices;
    while (head){
      if(ogg123_opts->verbosity>3)
        ao_append_global_option("debug",NULL);
      if(ogg123_opts->verbosity>2)
        ao_append_option(&(head->options),"verbose",NULL);
      if(ogg123_opts->verbosity==0)
        ao_append_option(&(head->options),"quiet",NULL);
      head = head->next_device;
    }
  }


  return optind;
}

#define LIST_SEP(x) ((x)==0?' ':',')

void cmdline_usage (void)
{
  int i, j, driver_count;
  ao_info **devices = ao_driver_info_list(&driver_count);

  printf (_("ogg123 from %s %s\n"
         " by the Xiph.Org Foundation (http://www.xiph.org/)\n\n"), PACKAGE, VERSION);

  printf (_("Usage: ogg123 [options] file ...\n"
	    "Play Ogg audio files and network streams.\n\n"));
 
  printf (_("Available codecs: "));

#ifdef HAVE_LIBFLAC
  printf (_("FLAC, "));
#endif

#ifdef HAVE_LIBSPEEX
  printf (_("Speex, "));
#endif

  printf (_("Ogg Vorbis.\n\n"));

  printf (_("Output options\n"));
  printf (_("  -d dev, --device dev    Use output device \"dev\". Available devices:\n"));
  printf ("                          ");
  printf (_("Live:"));

  for(i = 0, j = 0; i < driver_count; i++) {
    if (devices[i]->type == AO_TYPE_LIVE) {
      printf ("%c %s", LIST_SEP(j), devices[i]->short_name);
      j++;
    }
  }
  printf ("\n                          ");
  printf (_("File:"));
  for(i = 0, j = 0; i < driver_count; i++) {
    if (devices[i]->type == AO_TYPE_FILE) {
      printf ("%c %s", LIST_SEP(j), devices[i]->short_name);
      j++;
    }
  }
  printf ("\n\n");

  printf (_("  -f file, --file file    Set the output filename for a file device\n"
	    "                          previously specified with --device.\n"));
  printf ("\n");
  printf (_("  --audio-buffer n        Use an output audio buffer of 'n' kilobytes\n"));
  printf (_("  -o k:v, --device-option k:v\n"
	    "                          Pass special option 'k' with value 'v' to the\n"
	    "                          device previously specified with --device. See\n"
	    "                          the ogg123 man page for available device options.\n"));
  printf ("\n");

  printf (_("Playlist options\n"));
  printf (_("  -@ file, --list file    Read playlist of files and URLs from \"file\"\n"));
  printf (_("  -r, --repeat            Repeat playlist indefinitely\n"));
  printf (_("  -R, --remote            Use remote control interface\n"));
  printf (_("  -z, --shuffle           Shuffle list of files before playing\n"));
  printf (_("  -Z, --random            Play files randomly until interrupted\n"));
  printf ("\n");

  printf (_("Input options\n"));
  printf (_("  -b n, --buffer n        Use an input buffer of 'n' kilobytes\n"));
  printf (_("  -p n, --prebuffer n     Load n%% of the input buffer before playing\n"));
  printf ("\n");

  printf (_("Decode options\n"));
  printf (_("  -k n, --skip n          Skip the first 'n' seconds (or hh:mm:ss format)\n"));
  printf (_("  -K n, --end n           End at 'n' seconds (or hh:mm:ss format)\n"));
  printf (_("  -x n, --nth n           Play every 'n'th block\n"));
  printf (_("  -y n, --ntimes n        Repeat every played block 'n' times\n"));
  printf ("\n");

  printf (_("Miscellaneous options\n"));
  printf (_("  -l s, --delay s         Set termination timeout in milliseconds. ogg123\n"
	    "                          will skip to the next song on SIGINT (Ctrl-C),\n"
	    "                          and will terminate if two SIGINTs are received\n"
	    "                          within the specified timeout 's'. (default 500)\n"));
  printf ("\n");
  printf (_("  -h, --help              Display this help\n"));
  printf (_("  -q, --quiet             Don't display anything (no title)\n"));
  printf (_("  -v, --verbose           Display progress and other status information\n"));
  printf (_("  -V, --version           Display ogg123 version\n"));
  printf ("\n");

}
