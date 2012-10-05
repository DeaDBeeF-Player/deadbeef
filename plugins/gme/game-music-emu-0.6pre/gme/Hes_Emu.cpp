// Game_Music_Emu 0.6-pre. http://www.slack.net/~ant/

#include "Hes_Emu.h"

#include "blargg_endian.h"

/* Copyright (C) 2006-2008 Shay Green. This module is free software; you
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

Hes_Emu::Hes_Emu()
{
	set_type( gme_hes_type );
	set_silence_lookahead( 6 );
	set_gain( 1.11 );
}

Hes_Emu::~Hes_Emu() { }

void Hes_Emu::unload()
{
	core.unload();
	Music_Emu::unload();
}

static byte const* copy_field( byte const in [], char* out )
{
	if ( in )
	{
		int len = 0x20;
		if ( in [0x1F] && !in [0x2F] )
			len = 0x30; // fields are sometimes 16 bytes longer (ugh)
		
		// since text fields are where any data could be, detect non-text
		// and fields with data after zero byte terminator
		
		int i = 0;
		for ( ; i < len && in [i]; i++ )
			if ( (unsigned) (in [i] - ' ') >= 0xFF - ' ' ) // also treat 0xFF as non-text
				return 0; // non-ASCII found
		
		for ( ; i < len; i++ )
			if ( in [i] )
				return 0; // data after terminator
		
		Gme_File::copy_field_( out, (char const*) in, len );
		in += len;
	}
	return in;
}

static void copy_hes_fields( byte const in [], track_info_t* out )
{
	if ( *in >= ' ' )
	{
		in = copy_field( in, out->game      );
		in = copy_field( in, out->author    );
		in = copy_field( in, out->copyright );
	}
}

blargg_err_t Hes_Emu::track_info_( track_info_t* out, int ) const
{
	copy_hes_fields( core.data() + core.info_offset, out );
	return blargg_ok;
}

struct Hes_File : Gme_Info_
{
	enum { fields_offset = Hes_Core::header_t::size + Hes_Core::info_offset };
	
	union header_t {
		Hes_Core::header_t header;
		byte data [fields_offset + 0x30 * 3];
	} h;
	
	Hes_File()
	{
		set_type( gme_hes_type );
	}
	
	blargg_err_t load_( Data_Reader& in )
	{
		blargg_err_t err = in.read( &h, sizeof h );
		if ( err )
			return (blargg_is_err_type( err, blargg_err_file_eof ) ? blargg_err_file_type : err);
		
		if ( !h.header.valid_tag() )
			return blargg_err_file_type;
		
		return blargg_ok;
	}
	
	blargg_err_t track_info_( track_info_t* out, int ) const
	{
		copy_hes_fields( h.data + fields_offset, out );
		return blargg_ok;
	}
};

static Music_Emu* new_hes_emu () { return BLARGG_NEW Hes_Emu ; }
static Music_Emu* new_hes_file() { return BLARGG_NEW Hes_File; }

gme_type_t_ const gme_hes_type [1] = {{ "PC Engine", 256, &new_hes_emu, &new_hes_file, "HES", 1 }};

blargg_err_t Hes_Emu::load_( Data_Reader& in )
{
	RETURN_ERR( core.load( in ) );
	
	static const char* const names [Hes_Apu::osc_count + Hes_Apu_Adpcm::osc_count] = {
		"Wave 1", "Wave 2", "Wave 3", "Wave 4", "Multi 1", "Multi 2", "ADPCM"
	};
	set_voice_names( names );
	
	static int const types [Hes_Apu::osc_count + Hes_Apu_Adpcm::osc_count] = {
		wave_type+0, wave_type+1, wave_type+2, wave_type+3, mixed_type+0, mixed_type+1, mixed_type+2
	};
	set_voice_types( types );
	
	set_voice_count( core.apu().osc_count + core.adpcm().osc_count );
	core.apu().volume( gain() );
	core.adpcm().volume( gain() );
	
	return setup_buffer( 7159091 );
}

void Hes_Emu::update_eq( blip_eq_t const& eq )
{
	core.apu().treble_eq( eq );
}

void Hes_Emu::set_voice( int i, Blip_Buffer* c, Blip_Buffer* l, Blip_Buffer* r )
{
	if ( i < core.apu().osc_count )
		core.apu().set_output( i, c, l, r );
	else if ( i == core.apu().osc_count )
		core.adpcm().set_output( 0, c, l, r );
}

void Hes_Emu::set_tempo_( double t )
{
	core.set_tempo( t );
}

blargg_err_t Hes_Emu::start_track_( int track )
{
	RETURN_ERR( Classic_Emu::start_track_( track ) );
	return core.start_track( track );
}

blargg_err_t Hes_Emu::run_clocks( blip_time_t& duration_, int )
{
	return core.end_frame( duration_ );
}
