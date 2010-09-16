// Super Nintendo (SNES) SPC DSP emulator

// Game_Music_Emu 0.5.5
#ifndef SPC_DSP_H
#define SPC_DSP_H

#include "blargg_common.h"

class Spc_Dsp {
	typedef BOOST::int8_t int8_t;
	typedef BOOST::uint8_t uint8_t;
public:
	
	// Keeps pointer to 64K ram
	Spc_Dsp( uint8_t* ram );
	
	// Mute voice n if bit n (1 << n) of mask is clear.
	enum { voice_count = 8 };
	void mute_voices( int mask );
	
	// Clear state and silence everything.
	void reset();
	
	// Set gain, where 1.0 is normal. When greater than 1.0, output is clamped to
	// the 16-bit sample range.
	void set_gain( double );
	
	// If true, prevent channels and global volumes from being phase-negated
	void disable_surround( bool disable );
	
	// Read/write register 'n', where n ranges from 0 to register_count - 1.
	enum { register_count = 128 };
	int  read ( int n );
	void write( int n, int );
	
	// Run DSP for 'count' samples. Write resulting samples to 'buf' if not NULL.
	void run( long count, short* buf = NULL );
	
	
// End of public interface
private:
	
	struct raw_voice_t {
		int8_t  left_vol;
		int8_t  right_vol;
		uint8_t rate [2];
		uint8_t waveform;
		uint8_t adsr [2];   // envelope rates for attack, decay, and sustain
		uint8_t gain;       // envelope gain (if not using ADSR)
		int8_t  envx;       // current envelope level
		int8_t  outx;       // current sample
		int8_t  unused [6];
	};
	
	struct globals_t {
		int8_t  unused1 [12];
		int8_t  left_volume;        // 0C   Main Volume Left (-.7)
		int8_t  echo_feedback;      // 0D   Echo Feedback (-.7)
		int8_t  unused2 [14];
		int8_t  right_volume;       // 1C   Main Volume Right (-.7)
		int8_t  unused3 [15];
		int8_t  left_echo_volume;   // 2C   Echo Volume Left (-.7)
		uint8_t pitch_mods;         // 2D   Pitch Modulation on/off for each voice
		int8_t  unused4 [14];
		int8_t  right_echo_volume;  // 3C   Echo Volume Right (-.7)
		uint8_t noise_enables;      // 3D   Noise output on/off for each voice
		int8_t  unused5 [14];
		uint8_t key_ons;            // 4C   Key On for each voice
		uint8_t echo_ons;           // 4D   Echo on/off for each voice
		int8_t  unused6 [14];
		uint8_t key_offs;           // 5C   key off for each voice (instantiates release mode)
		uint8_t wave_page;          // 5D   source directory (wave table offsets)
		int8_t  unused7 [14];
		uint8_t flags;              // 6C   flags and noise freq
		uint8_t echo_page;          // 6D
		int8_t  unused8 [14];
		uint8_t wave_ended;         // 7C
		uint8_t echo_delay;         // 7D   ms >> 4
		char    unused9 [2];
	};
	
	union {
		raw_voice_t voice [voice_count];
		uint8_t reg [register_count];
		globals_t g;
	};
	
	uint8_t* const ram;
	
	// Cache of echo FIR values for faster access
	short fir_coeff [voice_count];
	
	// fir_buf [i + 8] == fir_buf [i], to avoid wrap checking in FIR code
	short fir_buf [16] [2];
	int fir_offset; // (0 to 7)
	
	enum { emu_gain_bits = 8 };
	int emu_gain;
	
	int keyed_on; // 8-bits for 8 voices
	int keys;
	
	int echo_ptr;
	int noise_amp;
	int noise;
	int noise_count;
	
	int surround_threshold;
	
	static BOOST::int16_t const gauss [];
	
	enum state_t {
		state_attack,
		state_decay,
		state_sustain,
		state_release
	};
	
	struct voice_t {
		short volume [2];
		short fraction;// 12-bit fractional position
		short interp3; // most recent four decoded samples
		short interp2;
		short interp1;
		short interp0;
		short block_remain; // number of nybbles remaining in current block
		unsigned short addr;
		short block_header; // header byte from current block
		short envcnt;
		short envx;
		short on_cnt;
		short enabled; // 7 if enabled, 31 if disabled
		short envstate;
		short unused; // pad to power of 2
	};
	
	voice_t voice_state [voice_count];
	
	int clock_envelope( int );
};

inline void Spc_Dsp::disable_surround( bool disable ) { surround_threshold = disable ? 0 : -0x7FFF; }

inline void Spc_Dsp::set_gain( double v ) { emu_gain = (int) (v * (1 << emu_gain_bits)); }

inline int Spc_Dsp::read( int i )
{
	assert( (unsigned) i < register_count );
	return reg [i];
}

#endif
