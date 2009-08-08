/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <SDL/SDL.h>
#include "psdl.h"
#include "streamer.h"
#include "common.h"

static int sdl_player_numsamples = 4096;
static int sdl_player_freq;
static SDL_AudioSpec spec;
static void psdl_callback (void *userdata, Uint8 *stream, int len);
static float sdl_volume = 1;

static inline void
le_int16 (int16_t in, char *out) {
    char *pin = (char *)&in;
#if !BIGENDIAN
    out[0] = pin[0];
    out[1] = pin[1];
#else
    out[1] = pin[0];
    out[0] = pin[1];
#endif
}

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

int
psdl_get_rate (void) {
    return sdl_player_freq;
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

