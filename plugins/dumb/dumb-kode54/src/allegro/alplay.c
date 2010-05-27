/*  _______         ____    __         ___    ___
 * \    _  \       \    /  \  /       \   \  /   /       '   '  '
 *  |  | \  \       |  |    ||         |   \/   |         .      .
 *  |  |  |  |      |  |    ||         ||\  /|  |
 *  |  |  |  |      |  |    ||         || \/ |  |         '  '  '
 *  |  |  |  |      |  |    ||         ||    |  |         .      .
 *  |  |_/  /        \  \__//          ||    |  |
 * /_______/ynamic    \____/niversal  /__\  /____\usic   /|  .  . ibliotheque
 *                                                      /  \
 *                                                     / .  \
 * alplay.c - Functions to play a DUH through         / / \  \
 *            an Allegro audio stream.               | <  /   \_
 *                                                   |  \/ /\   /
 * By entheh.                                         \_  /  > /
 *                                                      | \ / /
 *                                                      |  ' /
 *                                                       \__/
 */

#include <stdlib.h>

#include <allegro.h>

#include "aldumb.h"



#define ADP_PLAYING 1

struct AL_DUH_PLAYER
{
	int flags;
	long bufsize;
	int freq;
	AUDIOSTREAM *stream;
	DUH_SIGRENDERER *sigrenderer; /* If this is NULL, stream is invalid. */
	float volume;
	int silentcount;
};



AL_DUH_PLAYER *al_start_duh(DUH *duh, int n_channels, long pos, float volume, long bufsize, int freq)
{
	AL_DUH_PLAYER *dp;

	/* This restriction is imposed by Allegro. */
	ASSERT(n_channels > 0);
	ASSERT(n_channels <= 2);

	if (!duh)
		return NULL;

	dp = malloc(sizeof(*dp));
	if (!dp)
		return NULL;

	dp->flags = ADP_PLAYING;
	dp->bufsize = bufsize;
	dp->freq = freq;

	dp->stream = play_audio_stream(bufsize, 16, n_channels - 1, freq, 255, 128);

	if (!dp->stream) {
		free(dp);
		return NULL;
	}

	voice_set_priority(dp->stream->voice, 255);

	dp->sigrenderer = duh_start_sigrenderer(duh, 0, n_channels, pos);

	if (!dp->sigrenderer) {
		stop_audio_stream(dp->stream);
		free(dp);
		return NULL;
	}

	dp->volume = volume;
	dp->silentcount = 0;

	return dp;
}



void al_stop_duh(AL_DUH_PLAYER *dp)
{
	if (dp) {
		if (dp->sigrenderer) {
			duh_end_sigrenderer(dp->sigrenderer);
			stop_audio_stream(dp->stream);
		}
		free(dp);
	}
}



void al_pause_duh(AL_DUH_PLAYER *dp)
{
	if (dp && dp->sigrenderer && (dp->flags & ADP_PLAYING)) {
		voice_stop(dp->stream->voice);
		dp->flags &= ~ADP_PLAYING;
	}
}



void al_resume_duh(AL_DUH_PLAYER *dp)
{
	if (dp && dp->sigrenderer && !(dp->flags & ADP_PLAYING)) {
		voice_start(dp->stream->voice);
		dp->flags |= ADP_PLAYING;
	}
}



void al_duh_set_priority(AL_DUH_PLAYER *dp, int priority)
{
	if (dp && dp->sigrenderer)
		voice_set_priority(dp->stream->voice, priority);
}



void al_duh_set_volume(AL_DUH_PLAYER *dp, float volume)
{
	if (dp)
		dp->volume = volume;
}



float al_duh_get_volume(AL_DUH_PLAYER *dp)
{
	return dp ? dp->volume : 0;
}



float al_duh_get_volume(AL_DUH_PLAYER *dp)
{
	return dp ? dp->volume : 0;
}



int al_poll_duh(AL_DUH_PLAYER *dp)
{
	unsigned short *sptr;
	long n;
	long size;
	int n_channels;

	if (!dp || !dp->sigrenderer)
		return 1;

	if (!(dp->flags & ADP_PLAYING))
		return 0;

	sptr = get_audio_stream_buffer(dp->stream);

	if (!sptr)
		return 0;

	n = duh_render(dp->sigrenderer, 16, 1, dp->volume, 65536.0 / dp->freq, dp->bufsize, sptr);

	if (n == 0) {
		if (++dp->silentcount >= 2) {
			duh_end_sigrenderer(dp->sigrenderer);
			free_audio_stream_buffer(dp->stream);
			stop_audio_stream(dp->stream);
			dp->sigrenderer = NULL;
			return 1;
		}
	}

	n_channels = duh_sigrenderer_get_n_channels(dp->sigrenderer);
	n *= n_channels;
	size = dp->bufsize * n_channels;
	for (; n < size; n++)
		sptr[n] = 0x8000;

	free_audio_stream_buffer(dp->stream);

	return 0;
}



long al_duh_get_position(AL_DUH_PLAYER *dp)
{
	return dp ? duh_sigrenderer_get_position(dp->sigrenderer) : -1;
}



AL_DUH_PLAYER *al_duh_encapsulate_sigrenderer(DUH_SIGRENDERER *sigrenderer, float volume, long bufsize, int freq)
{
	AL_DUH_PLAYER *dp;
	int n_channels;

	if (!sigrenderer)
		return NULL;

	dp = malloc(sizeof(*dp));
	if (!dp)
		return NULL;

	n_channels = duh_sigrenderer_get_n_channels(sigrenderer);

	/* This restriction is imposed by Allegro. */
	ASSERT(n_channels > 0);
	ASSERT(n_channels <= 2);

	dp->flags = ADP_PLAYING;
	dp->bufsize = bufsize;
	dp->freq = freq;

	dp->stream = play_audio_stream(bufsize, 16, n_channels - 1, freq, 255, 128);

	if (!dp->stream) {
		free(dp);
		return NULL;
	}

	voice_set_priority(dp->stream->voice, 255);

	dp->sigrenderer = sigrenderer;

	dp->volume = volume;
	dp->silentcount = 0;

	return dp;
}



DUH_SIGRENDERER *al_duh_get_sigrenderer(AL_DUH_PLAYER *dp)
{
	return dp ? dp->sigrenderer : NULL;
}



/* IMPORTANT: This function will return NULL if the music has ended. */
// Should this be changed? User might want to hack the underlying SIGRENDERER
// and resurrect it (e.g. change pattern number), before it gets destroyed...
DUH_SIGRENDERER *al_duh_decompose_to_sigrenderer(AL_DUH_PLAYER *dp)
{
	if (dp) {
		DUH_SIGRENDERER *sigrenderer = dp->sigrenderer;
		if (sigrenderer) stop_audio_stream(dp->stream);
		free(dp);
		return sigrenderer;
	}
	return NULL;
}



/* DEPRECATED */
AL_DUH_PLAYER *al_duh_encapsulate_renderer(DUH_SIGRENDERER *dr, float volume, long bufsize, int freq)
{
	return al_duh_encapsulate_sigrenderer(dr, volume, bufsize, freq);
}



/* DEPRECATED */
DUH_SIGRENDERER *al_duh_get_renderer(AL_DUH_PLAYER *dp)
{
	return al_duh_get_sigrenderer(dp);
}



/* DEPRECATED */
DUH_SIGRENDERER *al_duh_decompose_to_renderer(AL_DUH_PLAYER *dp)
{
	return al_duh_decompose_to_sigrenderer(dp);
}
