// Super Nintendo (SNES) SPC-700 CPU emulator

// Game_Music_Emu 0.5.5
#ifndef SPC_CPU_H
#define SPC_CPU_H

#include "blargg_common.h"

typedef unsigned spc_addr_t;
typedef blargg_long spc_time_t;

class Snes_Spc;

class Spc_Cpu {
	typedef BOOST::uint8_t uint8_t;
	uint8_t* const ram;
public:
	// Keeps pointer to 64K RAM
	Spc_Cpu( Snes_Spc* spc, uint8_t* ram );
	
	// SPC-700 registers. *Not* kept updated during a call to run().
	struct registers_t {
		long pc; // more than 16 bits to allow overflow detection
		uint8_t a;
		uint8_t x;
		uint8_t y;
		uint8_t status;
		uint8_t sp;
	} r;
	
	// Run CPU for at least 'count' cycles. Return the number of cycles remaining
	// when emulation stopped (negative if extra cycles were emulated). Emulation
	// stops when there are no more remaining cycles or an unhandled instruction
	// is encountered (STOP, SLEEP, and any others not yet implemented). In the
	// latter case, the return value is greater than zero.
	spc_time_t run( spc_time_t count );
	
	// Number of clock cycles remaining for current run() call
	spc_time_t remain() const;
	
	// Access memory as the emulated CPU does
	int  read ( spc_addr_t );
	void write( spc_addr_t, int );
	
private:
	// noncopyable
	Spc_Cpu( const Spc_Cpu& );
	Spc_Cpu& operator = ( const Spc_Cpu& );
	unsigned mem_bit( spc_addr_t );
	
	spc_time_t remain_;
	Snes_Spc& emu;
};

inline spc_time_t Spc_Cpu::remain() const { return remain_; }

#endif
