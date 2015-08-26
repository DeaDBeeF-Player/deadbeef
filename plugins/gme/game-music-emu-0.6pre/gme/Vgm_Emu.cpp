// Game_Music_Emu $vers. http://www.slack.net/~ant/

#include "Vgm_Emu.h"

#include "blargg_endian.h"
#include "blargg_common.h"

/* Copyright (C) 2003-2008 Shay Green. This module is free software; you
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

// FM emulators are internally quieter to avoid 16-bit overflow
double const fm_gain           = 3.0;
double const rolloff           = 0.990;
double const oversample_factor = 1.5;

Vgm_Emu::Vgm_Emu()
{
	muted_voices = 0;
	set_type( gme_vgm_type );
	set_max_initial_silence( 1 );
	set_silence_lookahead( 1 ); // tracks should already be trimmed
}

Vgm_Emu::~Vgm_Emu() { }

void Vgm_Emu::unload()
{
	core.unload();
}

// Track info

static byte const* skip_gd3_str( byte const in [], byte const* end )
{
	while ( end - in >= 2 )
	{
		in += 2;
		if ( !(in [-2] | in [-1]) )
			break;
	}
	return in;
}

static byte const* get_gd3_str( byte const* in, byte const* end, char field [] )
{
	byte const* mid = skip_gd3_str( in, end );
	int len = (int)((mid - in) / 2) - 1;
	if ( len > 0 )
	{
        char * in_utf8 = blargg_to_utf8( (blargg_wchar_t *) in );
		len = min( len, (int) Gme_File::max_field_ );
		field [len] = 0;
		for ( int i = 0; i < len; i++ )
            field [i] = in_utf8 [i];
        free(in_utf8);
	}
	return mid;
}

static byte const* get_gd3_pair( byte const* in, byte const* end, char field [] )
{
	return skip_gd3_str( get_gd3_str( in, end, field ), end );
}

static void parse_gd3( byte const in [], byte const* end, track_info_t* out )
{
	in = get_gd3_pair( in, end, out->song      );
	in = get_gd3_pair( in, end, out->game      );
	in = get_gd3_pair( in, end, out->system    );
	in = get_gd3_pair( in, end, out->author    );
	in = get_gd3_str ( in, end, out->copyright );
	in = get_gd3_pair( in, end, out->dumper    );
	in = get_gd3_str ( in, end, out->comment   );
}

int const gd3_header_size = 12;

static int check_gd3_header( byte const h [], int remain )
{
	if ( remain < gd3_header_size ) return 0;
	if ( memcmp( h, "Gd3 ", 4 ) ) return 0;
	if ( get_le32( h + 4 ) >= 0x200 ) return 0;
	
	int gd3_size = get_le32( h + 8 );
	if ( gd3_size > remain - gd3_header_size ) return 0;
	
	return gd3_size;
}

static void get_vgm_length( Vgm_Emu::header_t const& h, track_info_t* out )
{
	int length = get_le32( &h.lngTotalSamples ) * 10 / 441; // 1000 / 44100
	if ( length > 0 )
	{
		int loop = get_le32( &h.lngLoopSamples );
		if ( loop > 0 && get_le32( &h.lngLoopOffset ) )
		{
			out->loop_length  = loop * 10 / 441;
			out->intro_length = length - out->loop_length;
			check( out->loop_length <= length );
			// TODO: Also set out->length? We now have play_length for suggested play time.
		}
		else
		{
			out->length       = length;
			out->intro_length = length;
			out->loop_length  = 0;
		}
	}
}

blargg_err_t Vgm_Emu::track_info_( track_info_t* out, int ) const
{
	get_vgm_length( header(), out );
	
	int gd3_offset = get_le32( &header().lngGD3Offset );
	if ( gd3_offset <= 0 )
		return blargg_ok;
	
	byte const* gd3 = core.file_begin() + gd3_offset;
	int gd3_size = check_gd3_header( gd3, (int)(core.file_end() - gd3) );
	if ( gd3_size )
	{
		byte const* gd3_data = gd3 + gd3_header_size;
		parse_gd3( gd3_data, gd3_data + gd3_size, out );
	}
	
	return blargg_ok;
}

blargg_err_t Vgm_Emu::gd3_data( const unsigned char ** data, int * size )
{
	*data = 0;
	*size = 0;

	int gd3_offset = get_le32( &header().lngGD3Offset );
	if ( gd3_offset <= 0 )
		return blargg_ok;

	byte const* gd3 = core.file_begin() + gd3_offset;
	int gd3_size = check_gd3_header( gd3, (int)(core.file_end() - gd3) );
	if ( gd3_size )
	{
		*data = gd3;
		*size = gd3_size + gd3_header_size;
	}

	return blargg_ok;
}

static void hash_vgm_file( Vgm_Emu::header_t const& h, byte const* data, int data_size, Music_Emu::Hash_Function& out )
{
	out.hash_( (const byte *) &h.lngEOFOffset, sizeof(h.lngEOFOffset) );
	out.hash_( (const byte *) &h.lngVersion, sizeof(h.lngVersion) );
	out.hash_( (const byte *) &h.lngHzPSG, sizeof(h.lngHzPSG) );
	out.hash_( (const byte *) &h.lngHzYM2413, sizeof(h.lngHzYM2413) );
	out.hash_( (const byte *) &h.lngTotalSamples, sizeof(h.lngTotalSamples) );
	out.hash_( (const byte *) &h.lngLoopOffset, sizeof(h.lngLoopOffset) );
	out.hash_( (const byte *) &h.lngLoopSamples, sizeof(h.lngLoopSamples) );
	out.hash_( (const byte *) &h.lngRate, sizeof(h.lngRate) );
	out.hash_( (const byte *) &h.shtPSG_Feedback, sizeof(h.shtPSG_Feedback) );
	out.hash_( (const byte *) &h.bytPSG_SRWidth, sizeof(h.bytPSG_SRWidth) );
	out.hash_( (const byte *) &h.bytPSG_Flags, sizeof(h.bytPSG_Flags) );
	out.hash_( (const byte *) &h.lngHzYM2612, sizeof(h.lngHzYM2612) );
	out.hash_( (const byte *) &h.lngHzYM2151, sizeof(h.lngHzYM2151) );
	out.hash_( (const byte *) &h.lngDataOffset, sizeof(h.lngDataOffset) );
	out.hash_( (const byte *) &h.lngHzSPCM, sizeof(h.lngHzSPCM) );
	out.hash_( (const byte *) &h.lngSPCMIntf, sizeof(h.lngSPCMIntf) );
	out.hash_( (const byte *) &h.lngHzRF5C68, sizeof(h.lngHzRF5C68) );
	out.hash_( (const byte *) &h.lngHzYM2203, sizeof(h.lngHzYM2203) );
	out.hash_( (const byte *) &h.lngHzYM2608, sizeof(h.lngHzYM2608) );
	out.hash_( (const byte *) &h.lngHzYM2610, sizeof(h.lngHzYM2610) );
	out.hash_( (const byte *) &h.lngHzYM3812, sizeof(h.lngHzYM3812) );
	out.hash_( (const byte *) &h.lngHzYM3526, sizeof(h.lngHzYM3526) );
	out.hash_( (const byte *) &h.lngHzY8950, sizeof(h.lngHzY8950) );
	out.hash_( (const byte *) &h.lngHzYMF262, sizeof(h.lngHzYMF262) );
	out.hash_( (const byte *) &h.lngHzYMF278B, sizeof(h.lngHzYMF278B) );
	out.hash_( (const byte *) &h.lngHzYMF271, sizeof(h.lngHzYMF271) );
	out.hash_( (const byte *) &h.lngHzYMZ280B, sizeof(h.lngHzYMZ280B) );
	out.hash_( (const byte *) &h.lngHzRF5C164, sizeof(h.lngHzRF5C164) );
	out.hash_( (const byte *) &h.lngHzPWM, sizeof(h.lngHzPWM) );
	out.hash_( (const byte *) &h.lngHzAY8910, sizeof(h.lngHzAY8910) );
	out.hash_( (const byte *) &h.bytAYType, sizeof(h.bytAYType) );
	out.hash_( (const byte *) &h.bytAYFlag, sizeof(h.bytAYFlag) );
	out.hash_( (const byte *) &h.bytAYFlagYM2203, sizeof(h.bytAYFlagYM2203) );
	out.hash_( (const byte *) &h.bytAYFlagYM2608, sizeof(h.bytAYFlagYM2608) );
	out.hash_( (const byte *) &h.bytReserved2, sizeof(h.bytReserved2) );
	out.hash_( (const byte *) &h.lngHzGBDMG, sizeof(h.lngHzGBDMG) );
	out.hash_( (const byte *) &h.lngHzNESAPU, sizeof(h.lngHzNESAPU) );
	out.hash_( (const byte *) &h.lngHzMultiPCM, sizeof(h.lngHzMultiPCM) );
	out.hash_( (const byte *) &h.lngHzUPD7759, sizeof(h.lngHzUPD7759) );
	out.hash_( (const byte *) &h.lngHzOKIM6258, sizeof(h.lngHzOKIM6258) );
	out.hash_( (const byte *) &h.bytOKI6258Flags, sizeof(h.bytOKI6258Flags) );
	out.hash_( (const byte *) &h.bytK054539Flags, sizeof(h.bytK054539Flags) );
	out.hash_( (const byte *) &h.bytC140Type, sizeof(h.bytC140Type) );
	out.hash_( (const byte *) &h.bytReservedFlags, sizeof(h.bytReservedFlags) );
	out.hash_( (const byte *) &h.lngHzOKIM6295, sizeof(h.lngHzOKIM6295) );
	out.hash_( (const byte *) &h.lngHzK051649, sizeof(h.lngHzK051649) );
	out.hash_( (const byte *) &h.lngHzK054539, sizeof(h.lngHzK054539) );
	out.hash_( (const byte *) &h.lngHzHuC6280, sizeof(h.lngHzHuC6280) );
	out.hash_( (const byte *) &h.lngHzC140, sizeof(h.lngHzC140) );
	out.hash_( (const byte *) &h.lngHzK053260, sizeof(h.lngHzK053260) );
	out.hash_( (const byte *) &h.lngHzPokey, sizeof(h.lngHzPokey) );
	out.hash_( (const byte *) &h.lngHzQSound, sizeof(h.lngHzQSound) );
    out.hash_( (const byte *) &h.lngHzSCSP, sizeof(h.lngHzSCSP) );
    // out.hash_( (const byte *) &h.lngExtraOffset, sizeof(h.lngExtraOffset) );
    out.hash_( (const byte *) &h.lngHzWSwan, sizeof(h.lngHzWSwan) );
    out.hash_( (const byte *) &h.lngHzVSU, sizeof(h.lngHzVSU) );
    out.hash_( (const byte *) &h.lngHzSAA1099, sizeof(h.lngHzSAA1099) );
    out.hash_( (const byte *) &h.lngHzES5503, sizeof(h.lngHzES5503) );
    out.hash_( (const byte *) &h.lngHzES5506, sizeof(h.lngHzES5506) );
    out.hash_( (const byte *) &h.bytES5503Chns, sizeof(h.bytES5503Chns) );
    out.hash_( (const byte *) &h.bytES5506Chns, sizeof(h.bytES5506Chns) );
    out.hash_( (const byte *) &h.bytC352ClkDiv, sizeof(h.bytC352ClkDiv) );
    out.hash_( (const byte *) &h.bytESReserved, sizeof(h.bytESReserved) );
    out.hash_( (const byte *) &h.lngHzX1_010, sizeof(h.lngHzX1_010) );
    out.hash_( (const byte *) &h.lngHzC352, sizeof(h.lngHzC352) );
    out.hash_( (const byte *) &h.lngHzGA20, sizeof(h.lngHzGA20) );
	out.hash_( data, data_size );
}

struct VGM_FILE_mem
{
    VGM_FILE vf;
    const BOOST::uint8_t* buffer;
    UINT32 ptr;
    UINT32 size;
};

static int VGMF_mem_Read(VGM_FILE* f, void* out, UINT32 count)
{
    VGM_FILE_mem* mf = (VGM_FILE_mem *) f;
    if (count + mf->ptr > mf->size)
        count = mf->size - mf->ptr;
    memcpy(out, mf->buffer + mf->ptr, count);
    mf->ptr += count;
    return count;
}

static int VGMF_mem_Seek(VGM_FILE* f, UINT32 offset)
{
    VGM_FILE_mem* mf = (VGM_FILE_mem *) f;
    if (offset > mf->size)
        offset = mf->size;
    mf->ptr = offset;
    return 0;
}

static UINT32 VGMF_mem_GetSize(VGM_FILE* f)
{
    VGM_FILE_mem* mf = (VGM_FILE_mem *) f;
    return mf->size;
}

struct Vgm_File : Gme_Info_
{
	Vgm_Emu::header_t h;
	blargg_vector<byte> data;
	blargg_vector<byte> gd3;
	
	Vgm_File() { set_type( gme_vgm_type ); }
	
	blargg_err_t load_mem_( const byte* in, int file_size )
	{
        VGM_FILE_mem memFile;
        
        memFile.vf.Read = &VGMF_mem_Read;
        memFile.vf.Seek = &VGMF_mem_Seek;
        memFile.vf.GetSize = &VGMF_mem_GetSize;
        memFile.buffer = in;
        memFile.ptr = 0;
        memFile.size = file_size;
        
        if (!GetVGMFileInfo_Handle((VGM_FILE *) &memFile, &h, 0))
			return blargg_err_file_type;
		
		int data_offset = get_le32( &h.lngDataOffset );
		int data_size = file_size - data_offset;
		int gd3_offset = get_le32( &h.lngGD3Offset );

		if ( gd3_offset > 0 && gd3_offset > data_offset )
		{
			data_size = gd3_offset - data_offset;

			RETURN_ERR( data.resize( data_size ) );
            memcpy( data.begin(), in + data_offset, data_size );
		}

		int remain = file_size - gd3_offset;
		byte gd3_h [gd3_header_size];
		if ( gd3_offset > 0 && remain >= gd3_header_size )
		{
            memcpy( gd3_h, in + gd3_offset, sizeof gd3_h );
			int gd3_size = check_gd3_header( gd3_h, remain );
			if ( gd3_size )
			{
				RETURN_ERR( gd3.resize( gd3_size ) );
                memcpy( gd3.begin(), in + sizeof gd3_h + gd3_offset, gd3.size() );
			}

			if ( data_offset > gd3_offset )
			{
				RETURN_ERR( data.resize( data_size ) );
                memcpy( data.begin(), in + data_offset, data_size );
			}
		}

		return blargg_ok;
	}
	
	blargg_err_t track_info_( track_info_t* out, int ) const
	{
		get_vgm_length( h, out );
		if ( gd3.size() )
			parse_gd3( gd3.begin(), gd3.end(), out );
		return blargg_ok;
	}

	blargg_err_t hash_( Hash_Function& out ) const
	{
		hash_vgm_file( h, data.begin(), (int)(data.end() - data.begin()), out );
		return blargg_ok;
	}
};

static Music_Emu* new_vgm_emu () { return BLARGG_NEW Vgm_Emu ; }
static Music_Emu* new_vgm_file() { return BLARGG_NEW Vgm_File; }

gme_type_t_ const gme_vgm_type [1] = {{ "Sega SMS/Genesis", 1, &new_vgm_emu, &new_vgm_file, "VGM", 0 }};

gme_type_t_ const gme_vgz_type [1] = {{ "Sega SMS/Genesis", 1, &new_vgm_emu, &new_vgm_file, "VGZ", 0 }};

// Setup

void Vgm_Emu::set_tempo_( double t )
{
	core.set_tempo( t );
}

blargg_err_t Vgm_Emu::set_sample_rate_( int sample_rate )
{
	core.set_sample_rate(sample_rate);
    return blargg_ok;
}

void Vgm_Emu::mute_voices_( int mask )
{
	muted_voices = mask;
}

// Emulation

blargg_err_t Vgm_Emu::start_track_( int track )
{
	core.start_track();

	mute_voices_(muted_voices);
	
	return blargg_ok;
}

inline void Vgm_Emu::check_end()
{
	if ( core.track_ended() )
		set_track_ended();
}

blargg_err_t Vgm_Emu::play_( int count, sample_t out [] )
{
    core.play_(count, out);
    check_end();
    return blargg_ok;
}

blargg_err_t Vgm_Emu::hash_( Hash_Function& out ) const
{
	byte const* p = file_begin();
	byte const* e = file_end();
	int data_offset = get_le32( &header().lngDataOffset );
	if ( data_offset )
		p += data_offset;
	int gd3_offset = get_le32( &header().lngGD3Offset );
	if ( gd3_offset > 0 && gd3_offset > data_offset )
        e = file_begin() + gd3_offset;
	hash_vgm_file( header(), p, (int)(e - p), out );
	return blargg_ok;
}

blargg_err_t Vgm_Emu::load_mem_( const byte* in, int file_size )
{
    return core.load_mem(in, file_size);
}

blargg_err_t Vgm_Emu::skip_( int count )
{
	core.skip_(count);
	return blargg_ok;
}
