// Super Nintendo (SNES) SPC-700 APU Emulator

// Game_Music_Emu 0.5.5
#ifndef SNES_SPC_H
#define SNES_SPC_H

#include "blargg_common.h"
#include "Spc_Cpu.h"
#include "Spc_Dsp.h"

class Snes_Spc {
public:
	
	// Load copy of SPC data into emulator. Clear echo buffer if 'clear_echo' is true.
	enum { spc_file_size = 0x10180 };
	blargg_err_t load_spc( const void* spc, long spc_size );
	
	// Generate 'count' samples and optionally write to 'buf'. Count must be even.
	// Sample output is 16-bit 32kHz, signed stereo pairs with the left channel first.
	typedef short sample_t;
	blargg_err_t play( long count, sample_t* buf = NULL );
	
// Optional functionality
	
	// Load copy of state into emulator.
	typedef Spc_Cpu::registers_t registers_t;
	blargg_err_t load_state( const registers_t& cpu_state, const void* ram_64k,
		const void* dsp_regs_128 );
	
	// Clear echo buffer, useful because many tracks have junk in the buffer.
	void clear_echo();
	
	// Mute voice n if bit n (1 << n) of mask is set
	enum { voice_count = Spc_Dsp::voice_count };
	void mute_voices( int mask );
	
	// Skip forward by the specified number of samples (64000 samples = 1 second)
	blargg_err_t skip( long count );
	
	// Set gain, where 1.0 is normal. When greater than 1.0, output is clamped the
	// 16-bit sample range.
	void set_gain( double );
	
	// If true, prevent channels and global volumes from being phase-negated
	void disable_surround( bool disable = true );
	
	// Set 128 bytes to use for IPL boot ROM. Makes copy. Default is zero filled,
	// to avoid including copyrighted code from the SPC-700.
	void set_ipl_rom( const void* );
	
	void set_tempo( double );
	
public:
	Snes_Spc();
	typedef BOOST::uint8_t uint8_t;
private:
	// timers
	struct Timer
	{
		spc_time_t next_tick;
		int period;
		int count;
		int divisor;
		int enabled;
		int counter;
		
		void run_until_( spc_time_t );
		void run_until( spc_time_t time )
		{
			if ( time >= next_tick )
				run_until_( time );
		}
	};
	enum { timer_count = 3 };
	Timer timer [timer_count];

	// hardware
	int extra_cycles;
	spc_time_t time() const;
	int  read( spc_addr_t );
	void write( spc_addr_t, int );
	friend class Spc_Cpu;
	
	// dsp
	sample_t* sample_buf;
	sample_t* buf_end; // to do: remove this once possible bug resolved
	spc_time_t next_dsp;
	Spc_Dsp dsp;
	int keys_pressed;
	int keys_released;
	sample_t skip_sentinel [1]; // special value for play() passed by skip()
	void run_dsp( spc_time_t );
	void run_dsp_( spc_time_t );
	bool echo_accessed;
	void check_for_echo_access( spc_addr_t );
	
	// boot rom
	enum { rom_size = 64 };
	enum { rom_addr = 0xFFC0 };
	bool rom_enabled;
	void enable_rom( bool );
	
	// CPU and RAM (at end because it's large)
	Spc_Cpu cpu;
	uint8_t extra_ram [rom_size];
	struct {
		// padding to catch jumps before beginning or past end
		uint8_t padding1 [0x100];
		uint8_t ram [0x10000];
		uint8_t padding2 [0x100];
	} mem;
	uint8_t boot_rom [rom_size];
};

inline void Snes_Spc::disable_surround( bool disable ) { dsp.disable_surround( disable ); }

inline void Snes_Spc::mute_voices( int mask ) { dsp.mute_voices( mask ); }

inline void Snes_Spc::set_gain( double v ) { dsp.set_gain( v ); }

#endif
