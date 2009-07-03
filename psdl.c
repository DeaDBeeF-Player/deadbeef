#include <SDL/SDL.h>
#include "psdl.h"
#include "codec.h"
#include "playlist.h"

static int sdl_player_numsamples = 2<<16;
static int sdl_player_freq;
static float *sdl_buffer[2];
static SDL_AudioSpec spec;
static void psdl_callback (void *userdata, Uint8 *stream, int len);
static codec_t *codec;

int
psdl_init (void) {
	SDL_AudioSpec obt;
	int formats[] = { AUDIO_S16, -1 };
	int freqs[] = { 48000, 44100, -1 };
	const char *fmtnames[] = { "16 bit signed integer" };
	int fmt, frq;
	int success = 0;
    sdl_buffer[0] = NULL;
    sdl_buffer[1] = NULL;
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
	sdl_player_freq = obt.freq;
	fprintf (stderr, "SDL: got %d frame size (requested %d), %dHz\n", obt.samples, sdl_player_numsamples, sdl_player_freq);
	return 0;
}

void
psdl_free (void) {
	SDL_CloseAudio ();
	if (sdl_buffer[0]) {
        free (sdl_buffer[0]);
        sdl_buffer[0] = NULL;
    }
    if (sdl_buffer[1]) {
        free (sdl_buffer[1]);
        sdl_buffer[1] = NULL;
    }
}

int
psdl_play (struct playItem_s *it) {
    if (!it) {
        return -1;
    }
    codec = it->codec;
    if (codec->init (it->fname)) {
        return -1;
    }
	SDL_PauseAudio (0);
	return 0;
}

int
psdl_stop (void) {
    codec = NULL;
	SDL_PauseAudio (1);
}

int
psdl_pause (void) {
	SDL_PauseAudio (1);
}

static void
psdl_callback (void* userdata, Uint8 *stream, int len) {
    if (!codec) {
        memset (stream, 0, len);
    }
    else {
        codec->read (stream, len);
    }
}

