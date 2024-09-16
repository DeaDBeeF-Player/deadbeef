/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2010 Simon Peter, <dn.tlp@gmx.net>, et al.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * surroundopl.h - Wrapper class to provide a surround/harmonic effect
 *   for another OPL emulator, by Adam Nielsen <malvineous@shikadi.net>
 *
 * Stereo harmonic algorithm by Adam Nielsen <malvineous@shikadi.net>
 * Please give credit if you use this algorithm elsewhere :-)
 */

#ifndef H_ADPLUG_SURROUNDOPL
#define H_ADPLUG_SURROUNDOPL

#include <stdint.h> // for uintxx_t
#include "opl.h"

// The right-channel is increased in frequency by itself divided by this amount.
// The right value should not noticeably change the pitch, but it should provide
// a nice stereo harmonic effect.
#define FREQ_OFFSET 128.0//96.0

// Number of FNums away from the upper/lower limit before switching to the next
// block (octave.)  By rights it should be zero, but for some reason this seems
// to cut it too close and the transposed OPL doesn't hit the right note all the
// time.  Setting it higher means it will switch blocks sooner and that seems
// to help.  Don't set it too high or it'll get stuck in an infinite loop if
// one block is too high and the adjacent block is too low ;-)
#define NEWBLOCK_LIMIT  32

struct COPLprops {
	Copl *opl;
	bool use16bit; // false == 8bit
	bool stereo; // false == mono
};

class CSurroundopl: public Copl
{
	private:
		COPLprops oplA, oplB;
		short bufsize;
		short *lbuf, *rbuf;
		bool output16bit;
		uint8_t iFMReg[2][256];
		uint8_t iTweakedFMReg[2][256];
		uint8_t iCurrentTweakedBlock[2][9]; // Current value of the Block in the tweaked OPL chip
		uint8_t iCurrentFNum[2][9];         // Current value of the FNum in the tweaked OPL chip
		double offset;                      // User configurable frequency offset for surroundopl

	public:

		// Takes ownership of a->opl and b->opl
		CSurroundopl(COPLprops *a, COPLprops *b, bool output16bit);
		~CSurroundopl();

		void update(short *buf, int samples);
		void write(int reg, int val);

		void init();
		void setchip(int n);
		void set_offset(double offset);
};

#endif
