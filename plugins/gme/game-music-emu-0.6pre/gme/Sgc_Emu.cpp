// Game_Music_Emu 0.6-pre. http://www.slack.net/~ant/

#include "Sgc_Emu.h"

/* Copyright (C) 2009 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details. You should have received a copy of the GNU Lesser General Public
License along with this module; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA */

#include "blargg_source.h"

int const osc_count = Sms_Apu::osc_count + Sms_Fm_Apu::osc_count;

Sgc_Emu::Sgc_Emu()
{
	set_type( gme_sgc_type );
	set_silence_lookahead( 6 );
	set_gain( 1.2 );
}

Sgc_Emu::~Sgc_Emu() { }

void Sgc_Emu::unload()
{
	core_.unload();
	Music_Emu::unload();
}

// Track info

static void copy_sgc_fields( Sgc_Emu::header_t const& h, track_info_t* out )
{
	GME_COPY_FIELD( h, out, game );
	GME_COPY_FIELD( h, out, author );
	GME_COPY_FIELD( h, out, copyright );
}

blargg_err_t Sgc_Emu::track_info_( track_info_t* out, int ) const
{
	copy_sgc_fields( header(), out );
	return blargg_ok;
}

struct Sgc_File : Gme_Info_
{
	Sgc_Emu::header_t h;
	
	Sgc_File() { set_type( gme_sgc_type ); }
	
	blargg_err_t load_( Data_Reader& in )
	{
		blargg_err_t err = in.read( &h, h.size );
		if ( err )
			return (blargg_is_err_type( err, blargg_err_file_eof ) ? blargg_err_file_type : err);
		
		set_track_count( h.song_count );
		if ( !h.valid_tag() )
			return blargg_err_file_type;
		
		return blargg_ok;
	}
	
	blargg_err_t track_info_( track_info_t* out, int ) const
	{
		copy_sgc_fields( h, out );
		return blargg_ok;
	}
};

static Music_Emu* new_sgc_emu () { return BLARGG_NEW Sgc_Emu ; }
static Music_Emu* new_sgc_file() { return BLARGG_NEW Sgc_File; }

gme_type_t_ const gme_sgc_type [1] = {{ "Z80 PSG", 0, &new_sgc_emu, &new_sgc_file, "SGC", 1 }};

// Setup

blargg_err_t Sgc_Emu::load_( Data_Reader& in )
{
	RETURN_ERR( core_.load( in ) );
	set_warning( core_.warning() );
	set_track_count( header().song_count );
	set_voice_count( core_.sega_mapping() ? osc_count : core_.apu().osc_count );
	
	core_.apu   ().volume( gain() );
	core_.fm_apu().volume( gain() );
	
	static const char* const names [osc_count + 1] = {
		"Square 1", "Square 2", "Square 3", "Noise", "FM"
	};
	set_voice_names( names );
	
	static int const types [osc_count + 1] = {
		wave_type+1, wave_type+2, wave_type+3, mixed_type+1, mixed_type+2
	};
	set_voice_types( types );
	
	return setup_buffer( core_.clock_rate() );
}

void Sgc_Emu::update_eq( blip_eq_t const& eq )
{
	core_.apu   ().treble_eq( eq );
	core_.fm_apu().treble_eq( eq );
}

void Sgc_Emu::set_voice( int i, Blip_Buffer* c, Blip_Buffer* l, Blip_Buffer* r )
{
	if ( i < core_.apu().osc_count )
		core_.apu().set_output( i, c, l, r );
	else
		core_.fm_apu().set_output( c, l, r );
}

void Sgc_Emu::set_tempo_( double t )
{
	core_.set_tempo( t );
}

blargg_err_t Sgc_Emu::start_track_( int track )
{
	RETURN_ERR( core_.start_track( track ) );
	return Classic_Emu::start_track_( track );
}

blargg_err_t Sgc_Emu::run_clocks( blip_time_t& duration, int )
{
	RETURN_ERR( core_.end_frame( duration ) );
	set_warning( core_.warning() );
	return blargg_ok;
}
