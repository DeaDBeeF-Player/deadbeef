#include <SDL/SDL.h>
#include <samplerate.h>
#include "psdl.h"
#include "codec.h"
#include "playlist.h"
#include "messagepump.h"
#include "messages.h"

static int sdl_player_numsamples = 2<<25;
static int sdl_player_freq;
static char *sdl_buffer;
static float *sdl_fbuffer;
static float *sdl_srcbuffer;
static SDL_AudioSpec spec;
static void psdl_callback (void *userdata, Uint8 *stream, int len);
static codec_t *codec;

static SRC_STATE *src;
static SRC_DATA srcdata;
static int codecleft;

static float sdl_volume = 1;
static float sdl_eof;

int
psdl_init (void) {
	SDL_AudioSpec obt;
	int formats[] = { AUDIO_S16, -1 };
	int freqs[] = { 48000, 44100, -1 };
	const char *fmtnames[] = { "16 bit signed integer" };
	int fmt, frq;
	int success = 0;
	fprintf (stderr, "sdl_player_init\n");
	for (fmt = 0; formats[fmt] != -1; fmt++) {
        for (frq = 0; freqs[frq] != -1; frq++) {
            fprintf(stderr, "SDL: trying %s @ %dHz\n", fmtnames[fmt], freqs[frq]);
            spec.freq = freqs[frq];
            spec.format = formats[fmt];
            spec.channels = 2;
            spec.samples = sdl_player_numsamples;
            spec.callback = psdl_callback;
            if (SDL_OpenAudio(&spec, &obt) < 0) {
                fprintf(stderr, "SDL: couldn't open audio: %s\n", SDL_GetError());
            }
            else {
                success = 1;
                break;
            }
        }
    }
    if (!success) {
        return -1;
    }
	sdl_player_numsamples = obt.samples; //numsamples;
    // source samplerate may be up to 96KHz, so need bigger buffers
	sdl_buffer = malloc (sdl_player_numsamples * sizeof (uint16_t) * 2 * 2);
	sdl_fbuffer = malloc (sdl_player_numsamples * sizeof (float) * 2 * 2); 
	sdl_srcbuffer = malloc (sdl_player_numsamples * sizeof (float) * 2);
	sdl_player_freq = obt.freq;
	fprintf (stderr, "SDL: got %d frame size (requested %d), %dHz\n", obt.samples, sdl_player_numsamples, sdl_player_freq);

//    src = src_new (SRC_SINC_BEST_QUALITY, 2, NULL);
    src = src_new (SRC_LINEAR, 2, NULL);
    codecleft = 0;

	return 0;
}

void
psdl_free (void) {
	SDL_CloseAudio ();
	if (sdl_buffer) {
        free (sdl_buffer);
        sdl_buffer = NULL;
    }
    src_delete (src);
    src = NULL;
}

int
psdl_play (struct playItem_s *it) {
//    printf ("psdl_play\n");
    if (!it) {
        return -1;
    }
    codec = it->codec;
    sdl_eof = 0;
    if (codec->init (it->fname)) {
        return -1;
    }
	SDL_PauseAudio (0);
	return 0;
}

int
psdl_stop (void) {
	SDL_PauseAudio (1);
    if (codec) {
        codec->free ();
        codec = NULL;
    }
}

int
psdl_ispaused (void) {
    if (!codec) {
        return 0;
    }
    return (SDL_GetAudioStatus () == SDL_AUDIO_PAUSED);
}

int
psdl_pause (void) {
	SDL_PauseAudio (1);
	return 0;
}

int
psdl_unpause (void) {
	SDL_PauseAudio (0);
	return 0;
}

void
psdl_set_volume (float vol) {
    sdl_volume = vol;
}

static void
psdl_callback (void* userdata, Uint8 *stream, int len) {
    if (sdl_eof) {
        messagepump_push (M_SONGFINISHED, 0, 0, 0);
        memset (stream, 0, len);
        return;
    }

    int codecret = 0;
    if (!codec) {
        memset (stream, 0, len);
    }
    else if (codec->info.samplesPerSecond == sdl_player_freq) {
        codec_lock ();
        codecret = codec->read (stream, len);
        codec_unlock ();
    }
    else {
        int nsamples = len/4;
        // convert to codec samplerate
        // add 5 extra samples for SRC
        nsamples = nsamples * codec->info.samplesPerSecond / sdl_player_freq + 5;
        // read data at source samplerate (with some room for SRC)
        codec_lock ();
        codecret = codec->read (sdl_buffer, (nsamples - codecleft) * 2 * codec->info.channels);
        // convert to float, and apply soft volume
        int i;
        float *fbuffer = sdl_fbuffer + codecleft*2;
        if (codec->info.channels == 2) {
            for (i = 0; i < (nsamples - codecleft) * 2; i++) {
                fbuffer[i] = ((int16_t *)sdl_buffer)[i]/32767.f * sdl_volume;
            }
        }
        else if (codec->info.channels == 1) {
            for (i = 0; i < (nsamples - codecleft); i++) {
                fbuffer[i*2+0] = ((int16_t *)sdl_buffer)[i]/32767.f * sdl_volume;
                fbuffer[i*2+1] = fbuffer[i*2+0] * sdl_volume;
            }
        }
        // convert samplerate
        srcdata.data_in = sdl_fbuffer;
        srcdata.data_out = sdl_srcbuffer;
        srcdata.input_frames = nsamples;
        srcdata.output_frames = len/4;
        srcdata.src_ratio = (double)sdl_player_freq/codec->info.samplesPerSecond;
        srcdata.end_of_input = 0;
        src_process (src, &srcdata);
        // convert back to s16 format
        for (i = 0; i < len/2; i++) {
            ((int16_t*)stream)[i] = (int16_t)(sdl_srcbuffer[i]*32767.f);
        }
        // calculate how many unused input samples left
        codecleft = nsamples - srcdata.input_frames_used;
        // copy spare samples for next update
        memmove (sdl_fbuffer, &sdl_fbuffer[srcdata.input_frames_used*2], codecleft * 8);
        codec_unlock ();
    }
    sdl_eof = codecret == -1 ? 1 : 0;
}

