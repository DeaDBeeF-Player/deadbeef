/*
	sdl: audio output via SDL cross-platform API

	copyright 2006-9 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Nicholas J. Humfrey
*/

#include <math.h>

#include <SDL.h>

#include "audio.h"
#include "mpg123app.h"
#ifdef WIN32
#include <windows.h>
#endif

/* Including the sfifo code locally, to avoid module linkage issues. */
#define SFIFO_STATIC
#include "sfifo.c"

#include "debug.h"


#define SAMPLE_SIZE			(2)
#define FRAMES_PER_BUFFER	(256)
#define FIFO_DURATION		(0.5f)

/* The audio function callback takes the following parameters:
       stream:  A pointer to the audio buffer to be filled
       len:     The length (in bytes) of the audio buffer
*/
static void audio_callback_sdl(void *udata, Uint8 *stream, int len)
{
	audio_output_t *ao = (audio_output_t*)udata;
	sfifo_t *fifo = (sfifo_t*)ao->userptr;
	int bytes_read;
	int bytes_avail;

	bytes_avail = sfifo_used(fifo);
	if(bytes_avail < len) len = bytes_avail;

	/* Read audio from FIFO to SDL's buffer */
	bytes_read = sfifo_read( fifo, stream, len );

	if (len!=bytes_read)
	warning2("Error reading from the FIFO (wanted=%u, bytes_read=%u).\n", len, bytes_read);
}

static int open_sdl(audio_output_t *ao)
{
	sfifo_t *fifo = (sfifo_t*)ao->userptr;
	
	/* Open an audio I/O stream. */
	if (ao->rate > 0 && ao->channels >0 ) {
		size_t ringbuffer_len;
		SDL_AudioSpec wanted;
	
		/* L16 uncompressed audio data, using 16-bit signed representation in twos 
		   complement notation - system endian-ness. */
		wanted.format = AUDIO_S16SYS;
		wanted.samples = 1024;  /* Good low-latency value for callback */ 
		wanted.callback = audio_callback_sdl; 
		wanted.userdata = ao; 
		wanted.channels = ao->channels; 
		wanted.freq = ao->rate; 

		/* Open the audio device, forcing the desired format
		   Actually, it is still subject to constraints by hardware.
		   Need to have sample rate checked beforehand! SDL will
		   happily play 22 kHz files with 44 kHz hardware rate!
		   Same with channel count. No conversion. The manual is a bit
		   misleading on that (only talking about sample format, I guess). */
		if ( SDL_OpenAudio(&wanted, NULL) ) {
			error1("Couldn't open SDL audio: %s\n", SDL_GetError());
			return -1;
		}
		
		/* Initialise FIFO */
		ringbuffer_len = ao->rate * FIFO_DURATION * SAMPLE_SIZE *ao->channels;
		debug2( "Allocating %d byte ring-buffer (%f seconds)", (int)ringbuffer_len, (float)FIFO_DURATION);
		if (sfifo_init( fifo, ringbuffer_len )) error1( "Failed to initialise FIFO of size %d bytes", (int)ringbuffer_len );
	}
	
	return(0);
}


static int get_formats_sdl(audio_output_t *ao)
{
	/* Got no better idea than to just take 16 bit and run with it */
	return MPG123_ENC_SIGNED_16;
#if 0
	/*
		This code would "properly" test audio format support.
		But thing is, SDL will always say yes and amen to everything, but it takes
		an awful amount of time to get all the variants tested (about 2 seconds,
		for example). I have seen SDL builds that do proper format conversion
		behind your back, I have seen builds that do not. Every build seems to
		claim that it does, though. Just hope you're lucky and your SDL works.
		Otherwise, use a proper audio output API.
	*/
	SDL_AudioSpec wanted, got;

	/* Only implemented Signed 16-bit audio for now.
	   The SDL manual doesn't suggest more interesting formats
	   like S24 or S32 anyway. */
	wanted.format = AUDIO_S16SYS;
	wanted.samples = 1024;
	wanted.callback = audio_callback_sdl;
	wanted.userdata = ao;
	wanted.channels = ao->channels;
	wanted.freq = ao->rate;

	if(SDL_OpenAudio(&wanted, &got)) return 0;
	SDL_CloseAudio();
fprintf(stderr, "wanted rate: %li got rate %li\n", (long)wanted.freq, (long)got.freq);
	return (got.freq == ao->rate && got.channels == ao->channels)
		? MPG123_ENC_SIGNED_16
		: 0;
#endif
}


static int write_sdl(audio_output_t *ao, unsigned char *buf, int len)
{
	sfifo_t *fifo = (sfifo_t*)ao->userptr;

	/* Sleep for half the length of the FIFO */
	while (sfifo_space( fifo ) < len )
#ifdef WIN32
		Sleep( (FIFO_DURATION/2) * 1000);
#else
		usleep( (FIFO_DURATION/2) * 1000000 );
#endif
	
	/* Bung decoded audio into the FIFO 
		 SDL Audio locking probably isn't actually needed
		 as SFIFO claims to be thread safe...
	*/
	SDL_LockAudio();
	sfifo_write( fifo, buf, len);
	SDL_UnlockAudio();
	
	
	/* Unpause once the buffer is 50% full */
	if (sfifo_used(fifo) > (sfifo_size(fifo)*0.5) )
	SDL_PauseAudio(0);

	return len;
}

static int close_sdl(audio_output_t *ao)
{
	int stuff;
	sfifo_t *fifo = (sfifo_t*)ao->userptr;

	/* Wait at least until SDL emptied the FIFO. */
	while((stuff = sfifo_used(fifo))>0)
	{
		int msecs = stuff*1000/ao->rate;
		debug1("still stuff for about %i ms there", msecs);
#ifdef WIN32
		Sleep(msecs/2);
#else
		usleep(msecs*1000/2);
#endif
	}

	SDL_CloseAudio();
	
	/* Free up the memory used by the FIFO */
	sfifo_close( fifo );
	
	return 0;
}

static void flush_sdl(audio_output_t *ao)
{
	sfifo_t *fifo = (sfifo_t*)ao->userptr;

	SDL_PauseAudio(1);
	
	sfifo_flush( fifo );	
}


static int deinit_sdl(audio_output_t* ao)
{
	/* Free up memory */
	if (ao->userptr) {
		free( ao->userptr );
		ao->userptr = NULL;
	}

	/* Shut down SDL */
	SDL_Quit();

	/* Success */
	return 0;
}


static int init_sdl(audio_output_t* ao)
{
	if (ao==NULL) return -1;
	
	/* Set callbacks */
	ao->open = open_sdl;
	ao->flush = flush_sdl;
	ao->write = write_sdl;
	ao->get_formats = get_formats_sdl;
	ao->close = close_sdl;
	ao->deinit = deinit_sdl;
	
	/* Allocate memory */
	ao->userptr = malloc( sizeof(sfifo_t) );
	if (ao->userptr==NULL) {
		error( "Failed to allocated memory for FIFO structure" );
		return -1;
	}
	memset( ao->userptr, 0, sizeof(sfifo_t) );

	/* Initialise SDL */
	if (SDL_Init( SDL_INIT_AUDIO ) ) {
		error1("Failed to initialise SDL: %s\n", SDL_GetError());
		return -1;
	}

	/* Success */
	return 0;
}


/* 
	Module information data structure
*/
mpg123_module_t mpg123_output_module_info = {
	/* api_version */	MPG123_MODULE_API_VERSION,
	/* name */			"sdl",
	/* description */	"Output audio using SDL (Simple DirectMedia Layer).",
	/* revision */		"$Rev:$",
	/* handle */		NULL,
	
	/* init_output */	init_sdl,
};
