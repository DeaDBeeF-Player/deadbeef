// Game_Music_Emu 0.5.5. http://www.slack.net/~ant/

// Based on Brad Martin's OpenSPC DSP emulator

#include "Spc_Dsp.h"

#include "blargg_endian.h"
#include <string.h>

/* Copyright (C) 2002 Brad Martin */
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

#ifdef BLARGG_ENABLE_OPTIMIZER
	#include BLARGG_ENABLE_OPTIMIZER
#endif

Spc_Dsp::Spc_Dsp( uint8_t* ram_ ) : ram( ram_ )
{
	set_gain( 1.0 );
	mute_voices( 0 );
	disable_surround( false );
	
	assert( offsetof (globals_t,unused9 [2]) == register_count );
	assert( sizeof (voice) == register_count );
	blargg_verify_byte_order();
}

void Spc_Dsp::mute_voices( int mask )
{
	for ( int i = 0; i < voice_count; i++ )
		voice_state [i].enabled = (mask >> i & 1) ? 31 : 7;
}

void Spc_Dsp::reset()
{
	keys = 0;
	echo_ptr = 0;
	noise_count = 0;
	noise = 1;
	fir_offset = 0;
	
	g.flags = 0xE0; // reset, mute, echo off
	g.key_ons = 0;
	
	for ( int i = 0; i < voice_count; i++ )
	{
		voice_t& v = voice_state [i];
		v.on_cnt = 0;
		v.volume [0] = 0;
		v.volume [1] = 0;
		v.envstate = state_release;
	}
	
	memset( fir_buf, 0, sizeof fir_buf );
}

void Spc_Dsp::write( int i, int data )
{
	require( (unsigned) i < register_count );
	
	reg [i] = data;
	int high = i >> 4;
	switch ( i & 0x0F )
	{
		// voice volume
		case 0:
		case 1: {
			short* volume = voice_state [high].volume;
			int left  = (int8_t) reg [i & ~1];
			int right = (int8_t) reg [i |  1];
			volume [0] = left;
			volume [1] = right;
			// kill surround only if enabled and signs of volumes differ
			if ( left * right < surround_threshold )
			{
				if ( left < 0 )
					volume [0] = -left;
				else
					volume [1] = -right;
			}
			break;
		}
		
		// fir coefficients
		case 0x0F:
			fir_coeff [high] = (int8_t) data; // sign-extend
			break;
	}
}

// This table is for envelope timing.  It represents the number of counts
// that should be subtracted from the counter each sample period (32kHz).
// The counter starts at 30720 (0x7800). Each count divides exactly into
// 0x7800 without remainder.
const int env_rate_init = 0x7800;
static short const env_rates [0x20] =
{
	0x0000, 0x000F, 0x0014, 0x0018, 0x001E, 0x0028, 0x0030, 0x003C,
	0x0050, 0x0060, 0x0078, 0x00A0, 0x00C0, 0x00F0, 0x0140, 0x0180,
	0x01E0, 0x0280, 0x0300, 0x03C0, 0x0500, 0x0600, 0x0780, 0x0A00,
	0x0C00, 0x0F00, 0x1400, 0x1800, 0x1E00, 0x2800, 0x3C00, 0x7800
};

const int env_range = 0x800;

inline int Spc_Dsp::clock_envelope( int v )
{                               /* Return value is current 
								 * ENVX */
	raw_voice_t& raw_voice = this->voice [v];
	voice_t& voice = voice_state [v];
	
	int envx = voice.envx;
	if ( voice.envstate == state_release )
	{
		/*
		 * Docs: "When in the state of "key off". the "click" sound is 
		 * prevented by the addition of the fixed value 1/256" WTF???
		 * Alright, I'm going to choose to interpret that this way:
		 * When a note is keyed off, start the RELEASE state, which
		 * subtracts 1/256th each sample period (32kHz).  Note there's 
		 * no need for a count because it always happens every update. 
		 */
		envx -= env_range / 256;
		if ( envx <= 0 )
		{
			envx = 0;
			keys &= ~(1 << v);
			return -1;
		}
		voice.envx = envx;
		raw_voice.envx = envx >> 8;
		return envx;
	}
	
	int cnt = voice.envcnt;
	int adsr1 = raw_voice.adsr [0];
	if ( adsr1 & 0x80 )
	{
		switch ( voice.envstate )
		{
			case state_attack: {
				// increase envelope by 1/64 each step
				int t = adsr1 & 15;
				if ( t == 15 )
				{
					envx += env_range / 2;
				}
				else
				{
					cnt -= env_rates [t * 2 + 1];
					if ( cnt > 0 )
						break;
					envx += env_range / 64;
					cnt = env_rate_init;
				}
				if ( envx >= env_range )
				{
					envx = env_range - 1;
					voice.envstate = state_decay;
				}
				voice.envx = envx;
				break;
			}
			
			case state_decay: {
				// Docs: "DR... [is multiplied] by the fixed value
				// 1-1/256." Well, at least that makes some sense.
				// Multiplying ENVX by 255/256 every time DECAY is
				// updated. 
				cnt -= env_rates [((adsr1 >> 3) & 0xE) + 0x10];
				if ( cnt <= 0 )
				{
					cnt = env_rate_init;
					envx -= ((envx - 1) >> 8) + 1;
					voice.envx = envx;
				}
				int sustain_level = raw_voice.adsr [1] >> 5;
				
				if ( envx <= (sustain_level + 1) * 0x100 )
					voice.envstate = state_sustain;
				break;
			}
			
			case state_sustain:
				// Docs: "SR [is multiplied] by the fixed value 1-1/256."
				// Multiplying ENVX by 255/256 every time SUSTAIN is
				// updated. 
				cnt -= env_rates [raw_voice.adsr [1] & 0x1F];
				if ( cnt <= 0 )
				{
					cnt = env_rate_init;
					envx -= ((envx - 1) >> 8) + 1;
					voice.envx = envx;
				}
				break;
			
			case state_release:
				// handled above
				break;
		}
	}
	else
	{                           /* GAIN mode is set */
		/*
		 * Note: if the game switches between ADSR and GAIN modes
		 * partway through, should the count be reset, or should it
		 * continue from where it was? Does the DSP actually watch for 
		 * that bit to change, or does it just go along with whatever
		 * it sees when it performs the update? I'm going to assume
		 * the latter and not update the count, unless I see a game
		 * that obviously wants the other behavior.  The effect would
		 * be pretty subtle, in any case. 
		 */
		int t = raw_voice.gain;
		if (t < 0x80)
		{
			envx = voice.envx = t << 4;
		}
		else switch (t >> 5)
		{
		case 4:         /* Docs: "Decrease (linear): Subtraction
							 * of the fixed value 1/64." */
			cnt -= env_rates [t & 0x1F];
			if (cnt > 0)
				break;
			cnt = env_rate_init;
			envx -= env_range / 64;
			if ( envx < 0 )
			{
				envx = 0;
				if ( voice.envstate == state_attack )
					voice.envstate = state_decay;
			}
			voice.envx = envx;
			break;
		case 5:         /* Docs: "Drecrease <sic> (exponential):
							 * Multiplication by the fixed value
							 * 1-1/256." */
			cnt -= env_rates [t & 0x1F];
			if (cnt > 0)
				break;
			cnt = env_rate_init;
			envx -= ((envx - 1) >> 8) + 1;
			if ( envx < 0 )
			{
				envx = 0;
				if ( voice.envstate == state_attack )
					voice.envstate = state_decay;
			}
			voice.envx = envx;
			break;
		case 6:         /* Docs: "Increase (linear): Addition of
							 * the fixed value 1/64." */
			cnt -= env_rates [t & 0x1F];
			if (cnt > 0)
				break;
			cnt = env_rate_init;
			envx += env_range / 64;
			if ( envx >= env_range )
				envx = env_range - 1;
			voice.envx = envx;
			break;
		case 7:         /* Docs: "Increase (bent line): Addition
							 * of the constant 1/64 up to .75 of the
							 * constaint <sic> 1/256 from .75 to 1." */
			cnt -= env_rates [t & 0x1F];
			if (cnt > 0)
				break;
			cnt = env_rate_init;
			if ( envx < env_range * 3 / 4 )
				envx += env_range / 64;
			else
				envx += env_range / 256;
			if ( envx >= env_range )
				envx = env_range - 1;
			voice.envx = envx;
			break;
		}
	}
	voice.envcnt = cnt;
	raw_voice.envx = envx >> 4;
	return envx;
}

// Clamp n into range -32768 <= n <= 32767
inline int clamp_16( int n )
{
	if ( (BOOST::int16_t) n != n )
		n = BOOST::int16_t (0x7FFF - (n >> 31));
	return n;
}

void Spc_Dsp::run( long count, short* out_buf )
{
	// to do: make clock_envelope() inline so that this becomes a leaf function?
	
	// Should we just fill the buffer with silence? Flags won't be cleared
	// during this run so it seems it should keep resetting every sample.
	if ( g.flags & 0x80 )
		reset();
	
	struct src_dir {
		char start [2];
		char loop [2];
	};
	
	const src_dir* const sd = (src_dir*) &ram [g.wave_page * 0x100];
	
	int left_volume  = g.left_volume;
	int right_volume = g.right_volume;
	if ( left_volume * right_volume < surround_threshold )
		right_volume = -right_volume; // kill global surround
	left_volume  *= emu_gain;
	right_volume *= emu_gain;
	
	while ( --count >= 0 )
	{
		// Here we check for keys on/off.  Docs say that successive writes
		// to KON/KOF must be separated by at least 2 Ts periods or risk
		// being neglected.  Therefore DSP only looks at these during an
		// update, and not at the time of the write.  Only need to do this
		// once however, since the regs haven't changed over the whole
		// period we need to catch up with. 
		
		g.wave_ended &= ~g.key_ons; // Keying on a voice resets that bit in ENDX.
		
		if ( g.noise_enables )
		{
			noise_count -= env_rates [g.flags & 0x1F];
			if ( noise_count <= 0 )
			{
				noise_count = env_rate_init;
				
				noise_amp = BOOST::int16_t (noise * 2);
				
				// TODO: switch to Galios style
				int feedback = (noise << 13) ^ (noise << 14);
				noise = (feedback & 0x4000) | (noise >> 1);
			}
		}
		
		// What is the expected behavior when pitch modulation is enabled on
		// voice 0? Jurassic Park 2 does this. Assume 0 for now.
		blargg_long prev_outx = 0;
		
		int echol = 0;
		int echor = 0;
		int left = 0;
		int right = 0;
		for ( int vidx = 0; vidx < voice_count; vidx++ )
		{
			const int vbit = 1 << vidx;
			raw_voice_t& raw_voice = voice [vidx];
			voice_t& voice = voice_state [vidx];
			
			if ( voice.on_cnt && !--voice.on_cnt )
			{
				// key on
				keys |= vbit;
				voice.addr = GET_LE16( sd [raw_voice.waveform].start );
				voice.block_remain = 1;
				voice.envx = 0;
				voice.block_header = 0;
				voice.fraction = 0x3FFF; // decode three samples immediately
				voice.interp0 = 0; // BRR decoder filter uses previous two samples
				voice.interp1 = 0;
				
				// NOTE: Real SNES does *not* appear to initialize the
				// envelope counter to anything in particular. The first
				// cycle always seems to come at a random time sooner than 
				// expected; as yet, I have been unable to find any
				// pattern.  I doubt it will matter though, so we'll go
				// ahead and do the full time for now. 
				voice.envcnt = env_rate_init;
				voice.envstate = state_attack;
			}
			
			if ( g.key_ons & vbit & ~g.key_offs )
			{
				// voice doesn't come on if key off is set
				g.key_ons &= ~vbit;
				voice.on_cnt = 8;
			}
			
			if ( keys & g.key_offs & vbit )
			{
				// key off
				voice.envstate = state_release;
				voice.on_cnt = 0;
			}
			
			int envx;
			if ( !(keys & vbit) || (envx = clock_envelope( vidx )) < 0 )
			{
				raw_voice.envx = 0;
				raw_voice.outx = 0;
				prev_outx = 0;
				continue;
			}
			
			// Decode samples when fraction >= 1.0 (0x1000)
			for ( int n = voice.fraction >> 12; --n >= 0; )
			{
				if ( !--voice.block_remain )
				{
					if ( voice.block_header & 1 )
					{
						g.wave_ended |= vbit;
					
						if ( voice.block_header & 2 )
						{
							// verified (played endless looping sample and ENDX was set)
							voice.addr = GET_LE16( sd [raw_voice.waveform].loop );
						}
						else
						{
							// first block was end block; don't play anything (verified)
							goto sample_ended; // to do: find alternative to goto
						}
					}
					
					voice.block_header = ram [voice.addr++];
					voice.block_remain = 16; // nybbles
				}
				
				// if next block has end flag set, *this* block ends *early* (verified)
				if ( voice.block_remain == 9 && (ram [voice.addr + 5] & 3) == 1 &&
						(voice.block_header & 3) != 3 )
				{
			sample_ended:
					g.wave_ended |= vbit;
					keys &= ~vbit;
					raw_voice.envx = 0;
					voice.envx = 0;
					// add silence samples to interpolation buffer
					do
					{
						voice.interp3 = voice.interp2;
						voice.interp2 = voice.interp1;
						voice.interp1 = voice.interp0;
						voice.interp0 = 0;
					}
					while ( --n >= 0 );
					break;
				}
				
				int delta = ram [voice.addr];
				if ( voice.block_remain & 1 )
				{
					delta <<= 4; // use lower nybble
					voice.addr++;
				}
				
				// Use sign-extended upper nybble
				delta = int8_t (delta) >> 4;
				
				// For invalid ranges (D,E,F): if the nybble is negative,
				// the result is F000.  If positive, 0000. Nothing else
				// like previous range, etc seems to have any effect.  If
				// range is valid, do the shift normally.  Note these are
				// both shifted right once to do the filters properly, but 
				// the output will be shifted back again at the end.
				int shift = voice.block_header >> 4;
				delta = (delta << shift) >> 1;
				if ( shift > 0x0C )
					delta = (delta >> 14) & ~0x7FF;
				
				// One, two and three point IIR filters
				int smp1 = voice.interp0;
				int smp2 = voice.interp1;
				if ( voice.block_header & 8 )
				{
					delta += smp1;
					delta -= smp2 >> 1;
					if ( !(voice.block_header & 4) )
					{
						delta += (-smp1 - (smp1 >> 1)) >> 5;
						delta += smp2 >> 5;
					}
					else
					{
						delta += (-smp1 * 13) >> 7;
						delta += (smp2 + (smp2 >> 1)) >> 4;
					}
				}
				else if ( voice.block_header & 4 )
				{
					delta += smp1 >> 1;
					delta += (-smp1) >> 5;
				}
				
				voice.interp3 = voice.interp2;
				voice.interp2 = smp2;
				voice.interp1 = smp1;
				voice.interp0 = BOOST::int16_t (clamp_16( delta ) * 2); // sign-extend
			}
			
			// rate (with possible modulation)
			int rate = GET_LE16( raw_voice.rate ) & 0x3FFF;
			if ( g.pitch_mods & vbit )
				rate = (rate * (prev_outx + 32768)) >> 15;
			
			// Gaussian interpolation using most recent 4 samples
			int index = voice.fraction >> 2 & 0x3FC;
			voice.fraction = (voice.fraction & 0x0FFF) + rate;
			const BOOST::int16_t* table  = (BOOST::int16_t const*) ((char const*) gauss + index);
			const BOOST::int16_t* table2 = (BOOST::int16_t const*) ((char const*) gauss + (255*4 - index));
			int s = ((table  [0] * voice.interp3) >> 12) +
					((table  [1] * voice.interp2) >> 12) +
					((table2 [1] * voice.interp1) >> 12);
			s = (BOOST::int16_t) (s * 2);
			s += (table2 [0] * voice.interp0) >> 11 & ~1;
			int output = clamp_16( s );
			if ( g.noise_enables & vbit )
				output = noise_amp;
			
			// scale output and set outx values
			output = (output * envx) >> 11 & ~1;
			
			// output and apply muting (by setting voice.enabled to 31)
			// if voice is externally disabled (not a SNES feature)
			int l = (voice.volume [0] * output) >> voice.enabled;
			int r = (voice.volume [1] * output) >> voice.enabled;
			prev_outx = output;
			raw_voice.outx = int8_t (output >> 8);
			if ( g.echo_ons & vbit )
			{
				echol += l;
				echor += r;
			}
			left  += l;
			right += r;
		}
		// end of channel loop
		
		// main volume control
		left  = (left  * left_volume ) >> (7 + emu_gain_bits);
		right = (right * right_volume) >> (7 + emu_gain_bits);
		
		// Echo FIR filter
		
		// read feedback from echo buffer
		int echo_ptr = this->echo_ptr;
		uint8_t* echo_buf = &ram [(g.echo_page * 0x100 + echo_ptr) & 0xFFFF];
		echo_ptr += 4;
		if ( echo_ptr >= (g.echo_delay & 15) * 0x800 )
			echo_ptr = 0;
		int fb_left  = (BOOST::int16_t) GET_LE16( echo_buf     ); // sign-extend
		int fb_right = (BOOST::int16_t) GET_LE16( echo_buf + 2 ); // sign-extend
		this->echo_ptr = echo_ptr;
		
		// put samples in history ring buffer
		const int fir_offset = this->fir_offset;
		short (*fir_pos) [2] = &fir_buf [fir_offset];
		this->fir_offset = (fir_offset + 7) & 7; // move backwards one step
		fir_pos [0] [0] = (short) fb_left;
		fir_pos [0] [1] = (short) fb_right;
		fir_pos [8] [0] = (short) fb_left; // duplicate at +8 eliminates wrap checking below
		fir_pos [8] [1] = (short) fb_right;
		
		// FIR
		fb_left =       fb_left * fir_coeff [7] +
				fir_pos [1] [0] * fir_coeff [6] +
				fir_pos [2] [0] * fir_coeff [5] +
				fir_pos [3] [0] * fir_coeff [4] +
				fir_pos [4] [0] * fir_coeff [3] +
				fir_pos [5] [0] * fir_coeff [2] +
				fir_pos [6] [0] * fir_coeff [1] +
				fir_pos [7] [0] * fir_coeff [0];
		
		fb_right =     fb_right * fir_coeff [7] +
				fir_pos [1] [1] * fir_coeff [6] +
				fir_pos [2] [1] * fir_coeff [5] +
				fir_pos [3] [1] * fir_coeff [4] +
				fir_pos [4] [1] * fir_coeff [3] +
				fir_pos [5] [1] * fir_coeff [2] +
				fir_pos [6] [1] * fir_coeff [1] +
				fir_pos [7] [1] * fir_coeff [0];
		
		left  += (fb_left  * g.left_echo_volume ) >> 14;
		right += (fb_right * g.right_echo_volume) >> 14;
		
		// echo buffer feedback
		if ( !(g.flags & 0x20) )
		{
			echol += (fb_left  * g.echo_feedback) >> 14;
			echor += (fb_right * g.echo_feedback) >> 14;
			SET_LE16( echo_buf    , clamp_16( echol ) );
			SET_LE16( echo_buf + 2, clamp_16( echor ) );
		}
		
		if ( out_buf )
		{
			// write final samples
			
			left  = clamp_16( left  );
			right = clamp_16( right );
			
			int mute = g.flags & 0x40;
			
			out_buf [0] = (short) left;
			out_buf [1] = (short) right;
			out_buf += 2;
			
			// muting
			if ( mute )
			{
				out_buf [-2] = 0;
				out_buf [-1] = 0;
			}
		}
	}
}

// Base normal_gauss table is almost exactly (with an error of 0 or -1 for each entry):
// int normal_gauss [512];
// normal_gauss [i] = exp((i-511)*(i-511)*-9.975e-6)*pow(sin(0.00307096*i),1.7358)*1304.45

// Interleved gauss table (to improve cache coherency).
// gauss [i * 2 + j] = normal_gauss [(1 - j) * 256 + i]
const BOOST::int16_t Spc_Dsp::gauss [512] =
{
 370,1305, 366,1305, 362,1304, 358,1304, 354,1304, 351,1304, 347,1304, 343,1303,
 339,1303, 336,1303, 332,1302, 328,1302, 325,1301, 321,1300, 318,1300, 314,1299,
 311,1298, 307,1297, 304,1297, 300,1296, 297,1295, 293,1294, 290,1293, 286,1292,
 283,1291, 280,1290, 276,1288, 273,1287, 270,1286, 267,1284, 263,1283, 260,1282,
 257,1280, 254,1279, 251,1277, 248,1275, 245,1274, 242,1272, 239,1270, 236,1269,
 233,1267, 230,1265, 227,1263, 224,1261, 221,1259, 218,1257, 215,1255, 212,1253,
 210,1251, 207,1248, 204,1246, 201,1244, 199,1241, 196,1239, 193,1237, 191,1234,
 188,1232, 186,1229, 183,1227, 180,1224, 178,1221, 175,1219, 173,1216, 171,1213,
 168,1210, 166,1207, 163,1205, 161,1202, 159,1199, 156,1196, 154,1193, 152,1190,
 150,1186, 147,1183, 145,1180, 143,1177, 141,1174, 139,1170, 137,1167, 134,1164,
 132,1160, 130,1157, 128,1153, 126,1150, 124,1146, 122,1143, 120,1139, 118,1136,
 117,1132, 115,1128, 113,1125, 111,1121, 109,1117, 107,1113, 106,1109, 104,1106,
 102,1102, 100,1098,  99,1094,  97,1090,  95,1086,  94,1082,  92,1078,  90,1074,
  89,1070,  87,1066,  86,1061,  84,1057,  83,1053,  81,1049,  80,1045,  78,1040,
  77,1036,  76,1032,  74,1027,  73,1023,  71,1019,  70,1014,  69,1010,  67,1005,
  66,1001,  65, 997,  64, 992,  62, 988,  61, 983,  60, 978,  59, 974,  58, 969,
  56, 965,  55, 960,  54, 955,  53, 951,  52, 946,  51, 941,  50, 937,  49, 932,
  48, 927,  47, 923,  46, 918,  45, 913,  44, 908,  43, 904,  42, 899,  41, 894,
  40, 889,  39, 884,  38, 880,  37, 875,  36, 870,  36, 865,  35, 860,  34, 855,
  33, 851,  32, 846,  32, 841,  31, 836,  30, 831,  29, 826,  29, 821,  28, 816,
  27, 811,  27, 806,  26, 802,  25, 797,  24, 792,  24, 787,  23, 782,  23, 777,
  22, 772,  21, 767,  21, 762,  20, 757,  20, 752,  19, 747,  19, 742,  18, 737,
  17, 732,  17, 728,  16, 723,  16, 718,  15, 713,  15, 708,  15, 703,  14, 698,
  14, 693,  13, 688,  13, 683,  12, 678,  12, 674,  11, 669,  11, 664,  11, 659,
  10, 654,  10, 649,  10, 644,   9, 640,   9, 635,   9, 630,   8, 625,   8, 620,
   8, 615,   7, 611,   7, 606,   7, 601,   6, 596,   6, 592,   6, 587,   6, 582,
   5, 577,   5, 573,   5, 568,   5, 563,   4, 559,   4, 554,   4, 550,   4, 545,
   4, 540,   3, 536,   3, 531,   3, 527,   3, 522,   3, 517,   2, 513,   2, 508,
   2, 504,   2, 499,   2, 495,   2, 491,   2, 486,   1, 482,   1, 477,   1, 473,
   1, 469,   1, 464,   1, 460,   1, 456,   1, 451,   1, 447,   1, 443,   1, 439,
   0, 434,   0, 430,   0, 426,   0, 422,   0, 418,   0, 414,   0, 410,   0, 405,
   0, 401,   0, 397,   0, 393,   0, 389,   0, 385,   0, 381,   0, 378,   0, 374,
};
