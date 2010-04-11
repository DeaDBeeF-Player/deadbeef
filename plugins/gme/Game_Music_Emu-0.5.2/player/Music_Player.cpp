// Game_Music_Emu 0.5.2. http://www.slack.net/~ant/

#include "Music_Player.h"

#include "gme/Music_Emu.h"

#include <string.h>
#include <ctype.h>

/* Copyright (C) 2005-2006 by Shay Green. Permission is hereby granted, free of
charge, to any person obtaining a copy of this software module and associated
documentation files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell copies of the Software, and
to permit persons to whom the Software is furnished to do so, subject to the
following conditions: The above copyright notice and this permission notice
shall be included in all copies or substantial portions of the Software. THE
SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */

#include "blargg_source.h"

// Number of audio buffers per second. Adjust if you encounter audio skipping.
const int fill_rate = 45;

// Simple sound driver using SDL
typedef void (*sound_callback_t)( void* data, short* out, int count );
static const char* sound_init( long sample_rate, int buf_size, sound_callback_t, void* data );
static void sound_start();
static void sound_stop();
static void sound_cleanup();

Music_Player::Music_Player()
{
	emu_      = 0;
	scope_buf = 0;
	paused    = false;
}

blargg_err_t Music_Player::init( long rate )
{
	sample_rate = rate;
	
	int min_size = sample_rate * 2 / fill_rate;
	int buf_size = 512;
	while ( buf_size < min_size )
		buf_size *= 2;
	
	return sound_init( sample_rate, buf_size, fill_buffer, this );
}

void Music_Player::stop()
{
	sound_stop();
	delete emu_;
	emu_ = 0;
}

Music_Player::~Music_Player()
{
	stop();
	sound_cleanup();
}

blargg_err_t Music_Player::load_file( const char* path )
{
	stop();
	
	RETURN_ERR( gme_open_file( path, &emu_, sample_rate ) );
	
	char m3u_path [256 + 5];
	strncpy( m3u_path, path, 256 );
	m3u_path [256] = 0;
	char* p = strrchr( m3u_path, '.' );
	if ( !p )
		p = m3u_path + strlen( m3u_path );
	strcpy( p, ".m3u" );
	if ( emu_->load_m3u( m3u_path ) ) { } // ignore error
	
	return 0;
}

int Music_Player::track_count() const
{
	return emu_ ? emu_->track_count() : false;
}

blargg_err_t Music_Player::start_track( int track )
{
	if ( emu_ )
	{
		// Sound must not be running when operating on emulator
		sound_stop();
		RETURN_ERR( emu_->start_track( track ) );
		
		// Calculate track length
		if ( !emu_->track_info( &track_info_ ) )
		{
			if ( track_info_.length <= 0 )
				track_info_.length = track_info_.intro_length +
						track_info_.loop_length * 2;
		}
		if ( track_info_.length <= 0 )
			track_info_.length = (long) (2.5 * 60 * 1000);
		emu_->set_fade( track_info_.length );
		
		paused = false;
		sound_start();
	}
	return 0;
}

void Music_Player::pause( int b )
{
	paused = b;
	if ( b )
		sound_stop();
	else
		sound_start();
}

void Music_Player::suspend()
{
	if ( !paused )
		sound_stop();
}

void Music_Player::resume()
{
	if ( !paused )
		sound_start();
}

bool Music_Player::track_ended() const
{
	return emu_ ? emu_->track_ended() : false;
}

void Music_Player::set_stereo_depth( double tempo )
{
	suspend();
	gme_set_stereo_depth( emu_, tempo );
	resume();
}

void Music_Player::set_tempo( double tempo )
{
	suspend();
	emu_->set_tempo( tempo );
	resume();
}

void Music_Player::mute_voices( int mask )
{
	suspend();
	emu_->mute_voices( mask );
	emu_->ignore_silence( mask != 0 );
	resume();
}

void Music_Player::fill_buffer( void* data, sample_t* out, int count )
{
	Music_Player* self = (Music_Player*) data;
	if ( self->emu_ )
	{
		if ( self->emu_->play( count, out ) ) { } // ignore error
		
		if ( self->scope_buf )
			memcpy( self->scope_buf, out, self->scope_buf_size * sizeof *self->scope_buf );
	}
}

// Sound output driver using SDL

#include "SDL.h"

static sound_callback_t sound_callback;
static void* sound_callback_data;

static void sdl_callback( void* data, Uint8* out, int count )
{
	if ( sound_callback )
		sound_callback( sound_callback_data, (short*) out, count / 2 );
}

static const char* sound_init( long sample_rate, int buf_size,
		sound_callback_t cb, void* data )
{
	sound_callback = cb;
	sound_callback_data = data;
	
	static SDL_AudioSpec as; // making static clears all fields to 0
	as.freq     = sample_rate;
	as.format   = AUDIO_S16SYS;
	as.channels = 2;
	as.callback = sdl_callback;
	as.samples  = buf_size;
	if ( SDL_OpenAudio( &as, 0 ) < 0 )
	{
		const char* err = SDL_GetError();
		if ( !err )
			err = "Couldn't open SDL audio";
		return err;
	}
	
	return 0;
}

static void sound_start()
{
	SDL_PauseAudio( false );
}

static void sound_stop()
{
	SDL_PauseAudio( true );
	
	// be sure audio thread is not active
	SDL_LockAudio();
	SDL_UnlockAudio();
}

static void sound_cleanup()
{
	sound_stop();
	SDL_CloseAudio();
}
