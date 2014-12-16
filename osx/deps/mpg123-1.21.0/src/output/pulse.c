/*
	pulse: audio output using PulseAudio server

	copyright 2006-9 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Nicholas J. Humfrey
*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <pulse/simple.h>
#include <pulse/error.h>

#include "config.h"
#include "mpg123app.h"
#include "audio.h"
#include "module.h"
#include "debug.h"

static int open_pulse(audio_output_t *ao)
{
	int err;
	pa_simple* pas = NULL;
	pa_sample_spec ss;
	/* Check if already open ? */
	if (ao->userptr) {
		error("Pulse audio output is already open.");
		return -1;
	}

	/* Open an audio I/O stream. */
	/* When they are < 0, I shall set some default. */
	if(ao->rate < 0 || ao->format < 0 || ao->channels < 0)
	{
		ao->rate     = 44100;
		ao->channels = 2;
		ao->format   = MPG123_ENC_SIGNED_16;
	}

	/* Fill out pulse audio's data structure */
	ss.channels = ao->channels;
	ss.rate = ao->rate;

	switch(ao->format) {
		case MPG123_ENC_SIGNED_16:
#ifdef WORDS_BIGENDIAN
			ss.format=PA_SAMPLE_S16BE;
#else
			ss.format=PA_SAMPLE_S16LE;
#endif
		break;
		case MPG123_ENC_ALAW_8:
			ss.format=PA_SAMPLE_ALAW;
		break;
		case MPG123_ENC_ULAW_8:
			ss.format=PA_SAMPLE_ULAW;
		break;
		case MPG123_ENC_UNSIGNED_8:
			ss.format=PA_SAMPLE_U8;
		break;
		default:
			error1("Unsupported audio format: 0x%x", ao->format);
			return -1;
		break;
	}


	/* Perform the open */
	pas = pa_simple_new(
			NULL,				/* Use the default server */
			"mpg123",			/* Our application's name */
			PA_STREAM_PLAYBACK,
			ao->device,			/* Use the default device if NULL */
			"MPEG Audio",		/* Description of our stream */
			&ss,				/* Our sample format */
			NULL,				/* Use default channel map */
			NULL,				/* Use default buffering attributes */
			&err				/* Error result code */
	);

	if( pas == NULL ) {
		error1("Failed to open pulse audio output: %s", pa_strerror(err));
		return -1;
	}

	/* Store the pointer */
	ao->userptr = (void*)pas;
	return 0;
}


static int get_formats_pulse(audio_output_t *ao)
{
	/* Only implemented Signed 16-bit audio for now */
	return MPG123_ENC_SIGNED_16;
}


static int write_pulse(audio_output_t *ao, unsigned char *buf, int len)
{
	pa_simple *pas = (pa_simple*)ao->userptr;
	int ret, err;
	/* Doesn't return number of bytes but just success or not. */
	ret = pa_simple_write( pas, buf, len, &err );
	if(ret<0){ error1("Failed to write audio: %s", pa_strerror(err)); return -1; }

	return len; /* If successful, everything has been written. */
}

static int close_pulse(audio_output_t *ao)
{
	pa_simple *pas = (pa_simple*)ao->userptr;

	if (pas) {
		int err; /* Do we really want to handle errors here? End is the end. */
		pa_simple_drain(pas, &err);
		pa_simple_free(pas);
		ao->userptr = NULL;
	}
	
	return 0;
}

static void flush_pulse(audio_output_t *ao)
{
	pa_simple *pas = (pa_simple*)ao->userptr;
	
	if (pas) {
		int err;
		pa_simple_flush( pas, &err );	
		if (err) error1("Failed to flush audio: %s", pa_strerror(err));
	}
}


static int init_pulse(audio_output_t* ao)
{
	if (ao==NULL) return -1;

	/* Set callbacks */
	ao->open = open_pulse;
	ao->flush = flush_pulse;
	ao->write = write_pulse;
	ao->get_formats = get_formats_pulse;
	ao->close = close_pulse;

	/* Success */
	return 0;
}


/* 
	Module information data structure
*/
mpg123_module_t mpg123_output_module_info = {
	/* api_version */	MPG123_MODULE_API_VERSION,
	/* name */			"pulse",						
	/* description */	"Output audio using PulseAudio Server",
	/* revision */		"$Rev:$",						
	/* handle */		NULL,
	
	/* init_output */	init_pulse,						
};

