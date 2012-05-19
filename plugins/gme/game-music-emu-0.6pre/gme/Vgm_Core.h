// Sega VGM music file emulator core

// Game_Music_Emu 0.6-pre
#ifndef VGM_CORE_H
#define VGM_CORE_H

#include "Gme_Loader.h"
#include "Ym2612_Emu.h"
#include "Ym2413_Emu.h"
#include "Sms_Apu.h"
#include "Multi_Buffer.h"

	template<class Emu>
	class Ym_Emu : public Emu {
		int last_time;
		short* out;
		enum { disabled_time = -1 };
	public:
		Ym_Emu()                        { last_time = disabled_time; out = NULL; }
		void enable( bool b = true )    { last_time = b ? 0 : disabled_time; }
		bool enabled() const            { return last_time != disabled_time; }
		void begin_frame( short* buf )  { out = buf; last_time = 0; }
		
		int run_until( int time )
		{
			int count = time - last_time;
			if ( count > 0 )
			{
				if ( last_time < 0 )
					return false;
				last_time = time;
				short* p = out;
				out += count * Emu::out_chan_count;
				Emu::run( count, p );
			}
			return true;
		}
	};

class Vgm_Core : public Gme_Loader {
public:

	// VGM file header
	struct header_t
	{
		enum { size = 0x40 };
		
		char tag            [4];
		byte data_size      [4];
		byte version        [4];
		byte psg_rate       [4];
		byte ym2413_rate    [4];
		byte gd3_offset     [4];
		byte track_duration [4];
		byte loop_offset    [4];
		byte loop_duration  [4];
		byte frame_rate     [4];
		byte noise_feedback [2];
		byte noise_width;
		byte unused1;
		byte ym2612_rate    [4];
		byte ym2151_rate    [4];
		byte data_offset    [4];
		byte unused2        [8];
		
		// True if header has valid file signature
		bool valid_tag() const;
	};
	
	// Header for currently loaded file
	header_t const& header() const      { return *(header_t const*) file_begin(); }
	
	// Raw file data, for parsing GD3 tags
	byte const* file_begin() const      { return Gme_Loader::file_begin(); }
	byte const* file_end  () const      { return Gme_Loader::file_end(); }
	
	// If file uses FM, initializes FM sound emulator using *sample_rate. If
	// *sample_rate is zero, sets *sample_rate to the proper accurate rate and
	// uses that. The output of the FM sound emulator is resampled to the
	// final sampling rate.
	blargg_err_t init_fm( double* sample_rate );
	
	// True if any FM chips are used by file. Always false until init_fm()
	// is called.
	bool uses_fm() const                { return ym2612.enabled() || ym2413.enabled(); }
	
	// Adjusts music tempo, where 1.0 is normal. Can be changed while playing.
	// Loading a file resets tempo to 1.0.
	void set_tempo( double );
	
	// Starts track
	void start_track();
	
	// Runs PSG-only VGM for msec and returns number of clocks it ran for
	blip_time_t run_psg( int msec );
	
	// Plays FM for at most count samples into *out, and returns number of
	// samples actually generated (always even). Also runs PSG for blip_time.
	int play_frame( blip_time_t blip_time, int count, blip_sample_t out [] );
	
	// True if all of file data has been played
	bool track_ended() const            { return pos >= file_end(); }
	
	// PCM sound is always generated here
	Stereo_Buffer stereo_buf;
	Blip_Buffer * blip_buf;
	
	// PSG sound chip, for assigning to Blip_Buffer, and setting volume and EQ
	Sms_Apu psg;
	
	// PCM synth, for setting volume and EQ
	Blip_Synth_Fast pcm;
	
	// FM sound chips
	Ym_Emu<Ym2612_Emu> ym2612;
	Ym_Emu<Ym2413_Emu> ym2413;

// Implementation
public:
	Vgm_Core();

protected:
	virtual blargg_err_t load_mem_( byte const [], int );
	
private:
	//          blip_time_t // PSG clocks
	typedef int vgm_time_t; // 44100 per second, REGARDLESS of sample rate
	typedef int fm_time_t;  // FM sample count
	
	int vgm_rate;   // rate of log, 44100 normally, adjusted by tempo
	double fm_rate; // FM samples per second
	
	// VGM to FM time
	int fm_time_factor;     
	int fm_time_offset;
	fm_time_t to_fm_time( vgm_time_t ) const;
	
	// VGM to PSG time
	int blip_time_factor;
	blip_time_t to_psg_time( vgm_time_t ) const;
	
	// Current time and position in log
	vgm_time_t vgm_time;
	byte const* pos;
	byte const* loop_begin;
	
	// PCM
	byte const* pcm_data;   // location of PCM data in log
	byte const* pcm_pos;    // current position in PCM data
	int dac_amp;
	int dac_disabled;       // -1 if disabled
	void write_pcm( vgm_time_t, int amp );
	
	blip_time_t run( vgm_time_t );
	int run_ym2413( int time );
	int run_ym2612( int time );
	void update_fm_rates( int* ym2413_rate, int* ym2612_rate ) const;
};

#endif
