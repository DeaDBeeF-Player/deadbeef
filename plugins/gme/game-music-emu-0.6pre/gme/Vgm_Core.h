// Sega VGM music file emulator core

// Game_Music_Emu $vers
#ifndef VGM_CORE_H
#define VGM_CORE_H

#include "Gme_Loader.h"

#include "../vgmplay/VGMPlay/VGMPlay.h"

class Vgm_Core : public Gme_Loader {
public:
	typedef VGM_HEADER header_t;

	// VGM file header
	// Header for currently loaded file
	header_t const& header() const      { return _header; }

	// Raw file data, for parsing GD3 tags
	byte const* file_begin() const      { return Gme_Loader::file_begin(); }
	byte const* file_end  () const      { return Gme_Loader::file_end(); }

	// Adjusts music tempo, where 1.0 is normal. Can be changed while playing.
	// Loading a file resets tempo to 1.0.
	void set_tempo( double );

	void set_sample_rate( int r ) { sample_rate = r; }

	// Starts track
	void start_track();

	// Plays FM for at most count samples into *out, and returns number of
	// samples actually generated (always even).
	int play_( int count, short out [] );

	// True if all of file data has been played
	bool track_ended() const            { return !!vgmp->VGMEnd; }

	// Skips the specified number of samples
	void skip_( int count );
    
    int get_channel_count();
    
    char* get_voice_name(int channel);
    void free_voice_name(char *);
    
    void set_mute(int mask);

// Implementation
public:
	Vgm_Core();
	~Vgm_Core();

protected:
	virtual blargg_err_t load_mem_( byte const [], int );

private:
	int sample_rate;

	header_t _header;

	VGM_PLAYER* vgmp;
};

#endif
