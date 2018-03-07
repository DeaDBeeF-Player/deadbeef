// Sega VGM music file emulator

// Game_Music_Emu $vers
#ifndef VGM_EMU_H
#define VGM_EMU_H

#include "Classic_Emu.h"
#include "Dual_Resampler.h"
#include "Vgm_Core.h"

/* Emulates VGM music using SN76489/SN76496 PSG, and YM2612 and YM2413 FM sound chips.
Supports custom sound buffer and frequency equalization when VGM uses just the PSG. FM
sound chips can be run at their proper rates, or slightly higher to reduce aliasing on
high notes. A YM2413 is supported but not provided separately from the library. */
class Vgm_Emu : public Music_Emu {
public:

	// VGM file header (see Vgm_Core.h)
	typedef Vgm_Core::header_t header_t;
	
	// Header for currently loaded file
	header_t const& header() const                      { return core.header(); }

	blargg_err_t hash_( Hash_Function& ) const;

	// Gd3 tag for currently loaded file
	blargg_err_t gd3_data( const unsigned char ** data, int * size );
	
	static gme_type_t static_type()                     { return gme_vgm_type; }

// Implementation
public:
	Vgm_Emu();
	~Vgm_Emu();
	
protected:
	blargg_err_t track_info_( track_info_t*, int track ) const;
    blargg_err_t set_track_info_( const track_info_t*, int track );
	blargg_err_t load_mem_( byte const [], int );
	blargg_err_t set_sample_rate_( int sample_rate );
	blargg_err_t start_track_( int );
	blargg_err_t play_( int count, sample_t  []);
	blargg_err_t skip_( int count );
    blargg_err_t save_( gme_writer_t, void* );
	virtual void set_tempo_( double );
	virtual void mute_voices_( int mask );
	virtual void unload();
	
private:
	unsigned muted_voices;
	Vgm_Core core;
    
    blargg_vector<byte> original_header;
    blargg_vector<byte> data;
    track_info_t metadata;
    track_info_t metadata_j;
    
    bool voice_names_assigned_;
	
	void check_end();
};

#endif
