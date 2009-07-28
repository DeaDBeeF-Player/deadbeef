#include <SDL/SDL.h>
#include "psdl.h"
#include "streamer.h"
#include "common.h"

static int sdl_player_numsamples = 4096;
int sdl_player_freq;
static SDL_AudioSpec spec;
static void psdl_callback (void *userdata, Uint8 *stream, int len);
static float sdl_volume = 1;

int
psdl_init (void) {
	SDL_AudioSpec obt;
	int formats[] = { AUDIO_S16, -1 };
	int freqs[] = { 48000, 44100, 96000, 22050, -1 };
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
	sdl_player_freq = obt.freq;
	fprintf (stderr, "SDL: got %d frame size (requested %d), %dHz\n", obt.samples, sdl_player_numsamples, sdl_player_freq);

	return 0;
}

void
psdl_free (void) {
	SDL_CloseAudio ();
}

int
psdl_play (void) {
	SDL_PauseAudio (0);
	return 0;
}

int
psdl_stop (void) {
	SDL_PauseAudio (1);
}

int
psdl_ispaused (void) {
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
    int bytesread = streamer_read (stream, len);
    int ivolume = sdl_volume * 1000;
    for (int i = 0; i < bytesread/2; i++) {
        int16_t sample = (int16_t)(((int32_t)(((int16_t*)stream)[i])) * ivolume / 1000);
        le_int16 (sample, (char*)&(((int16_t*)stream)[i]));
    }
    if (bytesread < len) {
        memset (stream + bytesread, 0, len-bytesread);
    }
}

