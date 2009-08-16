// Game_Music_Emu 0.5.2. http://www.slack.net/~ant/

#include "Snes_Spc.h"

#include <string.h>

/* Copyright (C) 2004-2006 Shay Green. This module is free software; you
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

// always in the future (CPU time can go over 0, but not by this much)
int const timer_disabled_time = 127;

Snes_Spc::Snes_Spc() : dsp( mem.ram ), cpu( this, mem.ram )
{
	set_tempo( 1.0 );
	
	// Put STOP instruction around memory to catch PC underflow/overflow.
	memset( mem.padding1, 0xFF, sizeof mem.padding1 );
	memset( mem.padding2, 0xFF, sizeof mem.padding2 );
	
	// A few tracks read from the last four bytes of IPL ROM
	boot_rom [sizeof boot_rom - 2] = 0xC0;
	boot_rom [sizeof boot_rom - 1] = 0xFF;
	memset( boot_rom, 0, sizeof boot_rom - 2 );
}

void Snes_Spc::set_tempo( double t )
{
	int unit = (int) (16.0 / t + 0.5);
	
	timer [0].divisor = unit * 8; // 8 kHz
	timer [1].divisor = unit * 8; // 8 kHz
	timer [2].divisor = unit;     // 64 kHz
}

// Load

void Snes_Spc::set_ipl_rom( void const* in )
{
	memcpy( boot_rom, in, sizeof boot_rom );
}

blargg_err_t Snes_Spc::load_spc( const void* data, long size )
{
	struct spc_file_t {
		char    signature [27];
		char    unused [10];
		uint8_t pc [2];
		uint8_t a;
		uint8_t x;
		uint8_t y;
		uint8_t status;
		uint8_t sp;
		char    unused2 [212];
		uint8_t ram [0x10000];
		uint8_t dsp [128];
		uint8_t ipl_rom [128];
	};
	assert( offsetof (spc_file_t,ipl_rom) == spc_file_size );
	
	const spc_file_t* spc = (spc_file_t const*) data;
	
	if ( size < spc_file_size )
		return "Not an SPC file";
	
	if ( strncmp( spc->signature, "SNES-SPC700 Sound File Data", 27 ) != 0 )
		return "Not an SPC file";
	
	registers_t regs;
	regs.pc     = spc->pc [1] * 0x100 + spc->pc [0];
	regs.a      = spc->a;
	regs.x      = spc->x;
	regs.y      = spc->y;
	regs.status = spc->status;
	regs.sp     = spc->sp;
	
	if ( (unsigned long) size >= sizeof *spc )
		set_ipl_rom( spc->ipl_rom );
	
	const char* error = load_state( regs, spc->ram, spc->dsp );
	
	echo_accessed = false;
	
	return error;
}

void Snes_Spc::clear_echo()
{
	if ( !(dsp.read( 0x6C ) & 0x20) )
	{
		unsigned addr = 0x100 * dsp.read( 0x6D );
		size_t   size = 0x800 * dsp.read( 0x7D );
		memset( mem.ram + addr, 0xFF, min( size, sizeof mem.ram - addr ) );
	}
}

// Handle other file formats (emulator save states) in user code, not here.

blargg_err_t Snes_Spc::load_state( const registers_t& cpu_state, const void* new_ram,
		const void* dsp_state )
{
	// cpu
	cpu.r = cpu_state;
	
	// Allow DSP to generate one sample before code starts
	// (Tengai Makyo Zero, Tenjin's Table Toss first notes are lost since it
	// clears KON 31 cycles from starting execution. It works on the SNES
	// since the SPC player adds a few extra cycles delay after restoring
	// KON from the DSP registers at the end of an SPC file).
	extra_cycles = 32; 
	
	// ram
	memcpy( mem.ram, new_ram, sizeof mem.ram );
	memcpy( extra_ram, mem.ram + rom_addr, sizeof extra_ram );
	
	// boot rom (have to force enable_rom() to update it)
	rom_enabled = !(mem.ram [0xF1] & 0x80);
	enable_rom( !rom_enabled );
	
	// dsp
	dsp.reset();
	int i;
	for ( i = 0; i < Spc_Dsp::register_count; i++ )
		dsp.write( i, ((uint8_t const*) dsp_state) [i] );
	
	// timers
	for ( i = 0; i < timer_count; i++ )
	{
		Timer& t = timer [i];
		
		t.next_tick = 0;
		t.enabled = (mem.ram [0xF1] >> i) & 1;
		if ( !t.enabled )
			t.next_tick = timer_disabled_time;
		t.count = 0;
		t.counter = mem.ram [0xFD + i] & 15;
		
		int p = mem.ram [0xFA + i];
		t.period = p ? p : 0x100;
	}
	
	// Handle registers which already give 0 when read by setting RAM and not changing it.
	// Put STOP instruction in registers which can be read, to catch attempted CPU execution.
	mem.ram [0xF0] = 0;
	mem.ram [0xF1] = 0;
	mem.ram [0xF3] = 0xFF;
	mem.ram [0xFA] = 0;
	mem.ram [0xFB] = 0;
	mem.ram [0xFC] = 0;
	mem.ram [0xFD] = 0xFF;
	mem.ram [0xFE] = 0xFF;
	mem.ram [0xFF] = 0xFF;
	
	return 0; // success
}

// Hardware

// Current time starts negative and ends at 0
inline spc_time_t Snes_Spc::time() const
{
	return -cpu.remain();
}

// Keep track of next time to run and avoid a function call if it hasn't been reached.

// Timers

void Snes_Spc::Timer::run_until_( spc_time_t time )
{
	if ( !enabled )
		dprintf( "next_tick: %ld, time: %ld", (long) next_tick, (long) time );
	assert( enabled ); // when disabled, next_tick should always be in the future
	
	int elapsed = ((time - next_tick) / divisor) + 1;
	next_tick += elapsed * divisor;
	
	elapsed += count;
	if ( elapsed >= period ) // avoid unnecessary division
	{
		int n = elapsed / period;
		elapsed -= n * period;
		counter = (counter + n) & 15;
	}
	count = elapsed;
}

// DSP

const int clocks_per_sample = 32; // 1.024 MHz CPU clock / 32000 samples per second

void Snes_Spc::run_dsp_( spc_time_t time )
{
	int count = ((time - next_dsp) >> 5) + 1; // divide by clocks_per_sample
	sample_t* buf = sample_buf;
	if ( buf ) {
		sample_buf = buf + count * 2; // stereo
		assert( sample_buf <= buf_end );
	}
	next_dsp += count * clocks_per_sample;
	dsp.run( count, buf );
}

inline void Snes_Spc::run_dsp( spc_time_t time )
{
	if ( time >= next_dsp )
		run_dsp_( time );
}

// Debug-only check for read/write within echo buffer, since this might result in
// inaccurate emulation due to the DSP not being caught up to the present.
inline void Snes_Spc::check_for_echo_access( spc_addr_t addr )
{
	if ( !echo_accessed && !(dsp.read( 0x6C ) & 0x20) )
	{
		// ** If echo accesses are found that require running the DSP, cache
		// the start and end address on DSP writes to speed up checking.
		
		unsigned start = 0x100 * dsp.read( 0x6D );
		unsigned end = start + 0x800 * dsp.read( 0x7D );
		if ( start <= addr && addr < end ) {
			echo_accessed = true;
			dprintf( "Read/write at $%04X within echo buffer\n", (unsigned) addr );
		}
	}
}

// Read

int Snes_Spc::read( spc_addr_t addr )
{
	int result = mem.ram [addr];
	
	if ( (rom_addr <= addr && addr < 0xFFFC || addr >= 0xFFFE) && rom_enabled )
		dprintf( "Read from ROM: %04X -> %02X\n", addr, result );
	
	if ( unsigned (addr - 0xF0) < 0x10 )
	{
		assert( 0xF0 <= addr && addr <= 0xFF );
		
		// counters
		int i = addr - 0xFD;
		if ( i >= 0 )
		{
			Timer& t = timer [i];
			t.run_until( time() );
			int old = t.counter;
			t.counter = 0;
			return old;
		}
		
		// dsp
		if ( addr == 0xF3 )
		{
			run_dsp( time() );
			if ( mem.ram [0xF2] >= Spc_Dsp::register_count )
				dprintf( "DSP read from $%02X\n", (int) mem.ram [0xF2] );
			return dsp.read( mem.ram [0xF2] & 0x7F );
		}
		
		if ( addr == 0xF0 || addr == 0xF1 || addr == 0xF8 ||
				addr == 0xF9 || addr == 0xFA )
			dprintf( "Read from register $%02X\n", (int) addr );
		
		// Registers which always read as 0 are handled by setting mem.ram [reg] to 0
		// at startup then never changing that value.
		
		check(( check_for_echo_access( addr ), true ));
	}
	
	return result;
}


// Write

void Snes_Spc::enable_rom( bool enable )
{
	if ( rom_enabled != enable )
	{
		rom_enabled = enable;
		memcpy( mem.ram + rom_addr, (enable ? boot_rom : extra_ram), rom_size );
		// TODO: ROM can still get overwritten when DSP writes to echo buffer
	}
}

void Snes_Spc::write( spc_addr_t addr, int data )
{
	// first page is very common
	if ( addr < 0xF0 ) {
		mem.ram [addr] = (uint8_t) data;
	}
	else switch ( addr )
	{
		// RAM
		default:
			check(( check_for_echo_access( addr ), true ));
			if ( addr < rom_addr ) {
				mem.ram [addr] = (uint8_t) data;
			}
			else {
				extra_ram [addr - rom_addr] = (uint8_t) data;
				if ( !rom_enabled )
					mem.ram [addr] = (uint8_t) data;
			}
			break;
		
		// DSP
		//case 0xF2: // mapped to RAM
		case 0xF3: {
			run_dsp( time() );
			int reg = mem.ram [0xF2];
			if ( next_dsp > 0 ) {
				// skip mode
				
				// key press
				if ( reg == 0x4C )
					keys_pressed |= data & ~dsp.read( 0x5C );
				
				// key release
				if ( reg == 0x5C ) {
					keys_released |= data;
					keys_pressed &= ~data;
				}
			}
			if ( reg < Spc_Dsp::register_count ) {
				dsp.write( reg, data );
			}
			else {
				dprintf( "DSP write to $%02X\n", (int) reg );
			}
			break;
		}
		
		case 0xF0: // Test register
			dprintf( "Wrote $%02X to $F0\n", (int) data );
			break;
		
		// Config
		case 0xF1:
		{
			// timers
			for ( int i = 0; i < timer_count; i++ )
			{
				Timer& t = timer [i];
				if ( !(data & (1 << i)) ) {
					t.enabled = 0;
					t.next_tick = timer_disabled_time;
				}
				else if ( !t.enabled ) {
					// just enabled
					t.enabled = 1;
					t.counter = 0;
					t.count = 0;
					t.next_tick = time();
				}
			}
			
			// port clears
			if ( data & 0x10 ) {
				mem.ram [0xF4] = 0;
				mem.ram [0xF5] = 0;
			}
			if ( data & 0x20 ) {
				mem.ram [0xF6] = 0;
				mem.ram [0xF7] = 0;
			}
			
			enable_rom( (data & 0x80) != 0 );
			
			break;
		}
		
		// Ports
		case 0xF4:
		case 0xF5:
		case 0xF6:
		case 0xF7:
			// to do: handle output ports
			break;
		
		//case 0xF8: // verified on SNES that these are read/write (RAM)
		//case 0xF9:
		
		// Timers
		case 0xFA:
		case 0xFB:
		case 0xFC: {
			Timer& t = timer [addr - 0xFA];
			if ( (t.period & 0xFF) != data ) {
				t.run_until( time() );
				t.period = data ? data : 0x100;
			}
			break;
		}
		
		// Counters (cleared on write)
		case 0xFD:
		case 0xFE:
		case 0xFF:
			dprintf( "Wrote to counter $%02X\n", (int) addr );
			timer [addr - 0xFD].counter = 0;
			break;
	}
}

// Play

blargg_err_t Snes_Spc::skip( long count )
{
	if ( count > 4 * 32000L )
	{
		// don't run DSP for long durations (2-3 times faster)
		
		const long sync_count = 32000L * 2;
		
		// keep track of any keys pressed/released (and not subsequently released)
		keys_pressed = 0;
		keys_released = 0;
		// sentinel tells play to ignore DSP
		RETURN_ERR( play( count - sync_count, skip_sentinel ) );
		
		// press/release keys now
		dsp.write( 0x5C, keys_released & ~keys_pressed );
		dsp.write( 0x4C, keys_pressed );
		
		clear_echo();
		
		// play the last few seconds normally to help synchronize DSP
		count = sync_count;
	}
	
	return play( count );
}

blargg_err_t Snes_Spc::play( long count, sample_t* out )
{
	require( count % 2 == 0 ); // output is always in pairs of samples
	
	// CPU time() runs from -duration to 0
	spc_time_t duration = (count / 2) * clocks_per_sample;
	
	// DSP output is made on-the-fly when the CPU reads/writes DSP registers
	sample_buf = out;
	buf_end = out + (out && out != skip_sentinel ? count : 0);
	next_dsp = (out == skip_sentinel) ? clocks_per_sample : -duration + clocks_per_sample;
	
	// Localize timer next_tick times and run them to the present to prevent a running
	// but ignored timer's next_tick from getting too far behind and overflowing.
	for ( int i = 0; i < timer_count; i++ )
	{
		Timer& t = timer [i];
		if ( t.enabled )
		{
			t.next_tick -= duration;
			t.run_until( -duration );
		}
	}
	
	// Run CPU for duration, reduced by any extra cycles from previous run
	int elapsed = cpu.run( duration - extra_cycles );
	if ( elapsed > 0 )
	{
		dprintf( "Unhandled instruction $%02X, pc = $%04X\n",
				(int) cpu.read( cpu.r.pc ), (unsigned) cpu.r.pc );
		return "Emulation error (illegal/unsupported instruction)";
	}
	extra_cycles = -elapsed;
	
	// Catch DSP up to present.
	run_dsp( 0 );
	if ( out ) {
		assert( next_dsp == clocks_per_sample );
		assert( out == skip_sentinel || sample_buf - out == count );
	}
	buf_end = 0;
	
	return 0;
}
