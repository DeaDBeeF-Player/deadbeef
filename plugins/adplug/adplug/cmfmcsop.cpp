/*
 * SoundFX Macs Opera CMF Player --  Copyright (c) 2017 Sebastian Kienzl <seb@knzl.de>
 *
 * Part of Adplug - Replayer for many OPL2/OPL3 audio file formats.
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
 */

/*
 * Most songs in this format use rhythm mode.
 * The emus don't support rhythm mode too well, "nuked" seems good.
 * I didn't verify with real hardware, so I don't know how it's supposed to sound.
 */

#include <string.h>
#include <stddef.h>
#include "cmfmcsop.h"
#include "debug.h"

CPlayer* CcmfmacsoperaPlayer::factory(Copl* newopl)
{
	return new CcmfmacsoperaPlayer(newopl);
}

static const int16_t fNumbers[] = { 0x157, 0x16b, 0x181, 0x198, 0x1b0, 0x1ca, 0x1e5, 0x202, 0x221, 0x242, 0x264, 0x288 };

// Register offset for slot, e.g. 0x20 + slotRegisterOffsets[slotNr]
static const int8_t slotRegisterOffsets[] = {
	0,   1,  2,  3,  4,  5,
	8,   9, 10, 11, 12, 13,
	16, 17, 18, 19, 20, 21
};

/*
Channel     Slot
          Op1  Op2
===================
 0        0    3
 1        1    4
 2        2    5
 3        6    9
 4        7   10
 5        8   11
 6       12   15
 7       13   16
 8       14   17

 Rhythm Mode
-------------------
 BD      12   15   <- like Channel 6 in "melody mode"
 SD      16
 TOM     14
 TC      17
 HH      13
*/

// Slots for Op1/Op2 for channel
static const struct {
	int8_t slotOp1;
	int8_t slotOp2;
}
channelSlots[] = { {0, 3}, {1, 4}, {2, 5}, {6, 9}, {7, 10}, {8, 11}, {12, 15}, {13, 16}, {14, 17}};

/*
Note that
 op_table[chan]     == slotRegisterOffsets[channelSlots[chan].slotOp1]
 op_table[chan] + 3 == slotRegisterOffsets[channelSlots[chan].slotOp2]
However, the slot-numbers are needed for rhythm mode, thus the extra tables.
*/

// Rhythm mode mapping: Which slots for which channel (note that the BD uses 12/15 like in melody mode)
static const int8_t channelSlotsRhythm[] = { -1, -1, -1, -1, -1, -1, 12, 16, 14, 17, 13};

enum {
	CHANNEL_RHY_BD  = 6,
	CHANNEL_RHY_SN  = 7,
	CHANNEL_RHY_TOM = 8,
	CHANNEL_RHY_TC  = 9,
	CHANNEL_RHY_HH  = 10
};

CcmfmacsoperaPlayer::CcmfmacsoperaPlayer(Copl* newopl): CPlayer(newopl)
{
}

std::string CcmfmacsoperaPlayer::gettype() { return std::string("SoundFX Macs Opera CMF"); }

// helper class to close a binistream when leaving context
class binistream_closer {
public:
	binistream_closer(const CFileProvider& fp, binistream* strm): fp(fp), strm(strm) {}
	~binistream_closer() { if(strm) fp.close(strm); }
	const CFileProvider& fp;
	binistream* strm;
};

bool CcmfmacsoperaPlayer::load(const std::string& filename, const CFileProvider& fp)
{
	if (!fp.extension(filename, ".cmf"))
		return false;
	
	binistream* f = fp.open(filename);
	if (!f)
		return false;

	binistream_closer _closer(fp, f);
	
	std::string signature = f->readString();
	if (signature != "A.H.")
		return false;

	// the pattern order is 99 shorts
	nrOfOrders = -1;
	for (int i = 0; i < 99; ++i) {
		patternOrder[i] = f->readInt(2);
		// count number of orders, only assign once
		if (patternOrder[i] == 99 && nrOfOrders < 0)
			nrOfOrders = i;
	}

	if (nrOfOrders == -1) // no end of song marker found, assume 99
		nrOfOrders = 99;

	nrOfPatterns = f->readInt(2);
	
	int speed = f->readInt(2);
	if (speed < 1 || speed > 3)
		return false;
	
	// 1 => 18 rows/s, 2 => 9 rows/s, 3 => 4.5 rows/s
	// (With 18.2 the speed matches the speed with DOSBox, otherwise it's ~2 BPM too slow)
	speedRowsPerSec = 18.2f / (1 << (speed - 1));

	rhythmMode = f->readInt(2) == 1;
	int nrOfInstruments = f->readInt(2);
	
	if (!loadInstruments(f, nrOfInstruments))
		return false;
	
	if (!loadPatterns(f))
		return false;
	
	rewind(0);
	
	return true;
}

bool CcmfmacsoperaPlayer::loadInstruments(binistream* f, int nrOfInstruments)
{
	if (nrOfInstruments > 0xff)
		return false;

	instruments.resize(nrOfInstruments);

	static const intptr_t loadOffsets[] = {
		offsetof(Instrument, op[0].ksl),
		offsetof(Instrument, op[0].multiple),
		offsetof(Instrument, feedback),
		offsetof(Instrument, op[0].attackRate),
		offsetof(Instrument, op[0].sustainLevel),
		offsetof(Instrument, op[0].egType),
		offsetof(Instrument, op[0].decayRate),
		offsetof(Instrument, op[0].releaseRate),
		offsetof(Instrument, op[0].totalLevel),
		offsetof(Instrument, op[0].ampMod),
		offsetof(Instrument, op[0].vib),
		offsetof(Instrument, op[0].ksr),
		offsetof(Instrument, connection),

		offsetof(Instrument, op[1].ksl),
		offsetof(Instrument, op[1].multiple),
		-1,
		offsetof(Instrument, op[1].attackRate),
		offsetof(Instrument, op[1].sustainLevel),
		offsetof(Instrument, op[1].egType),
		offsetof(Instrument, op[1].decayRate),
		offsetof(Instrument, op[1].releaseRate),
		offsetof(Instrument, op[1].totalLevel),
		offsetof(Instrument, op[1].ampMod),
		offsetof(Instrument, op[1].vib),
		offsetof(Instrument, op[1].ksr),
		-1,
		offsetof(Instrument, op[0].waveSelect),
		offsetof(Instrument, op[1].waveSelect)
	};

	for (int i = 0; i < nrOfInstruments; ++i) {
		for (unsigned int j = 0; j < sizeof(loadOffsets)/sizeof(loadOffsets[0]); ++j) {
			int v = f->readInt(2);
			if (loadOffsets[j] >= 0 ) {
				*(int16_t*)((char*)&instruments[i] + loadOffsets[j]) = v;
			}
		}

		f->readString(instruments[i].name, 13);
		instruments[i].name[13] = 0;
	}

	return !f->ateof();
}

bool CcmfmacsoperaPlayer::loadPatterns(binistream* f)
{
	if (nrOfPatterns > 0xff)
		return false;

	patterns.resize(nrOfPatterns);

	for (int i = 0; i < nrOfPatterns; ++i) {
		while (!f->eof()) {
			// 6 bytes per event, if the 1st byte is 0xff, end of pattern
			NoteEvent n;
			
			n.row = f->readInt(1);
			if (n.row == 0xff)
				break;
			
			uint8_t* d = &n.col;
			for (int j = 0; j < 5; ++j)
				*d++ = f->readInt(1);

			n.instrument--;
			patterns[i].push_back(n);
		}
	}
	
	return true;
}

bool CcmfmacsoperaPlayer::isValidChannel(int channelNr) const
{
	return channelNr >= 0 && ((rhythmMode && channelNr < 11) || (!rhythmMode && channelNr < 9));
}

bool CcmfmacsoperaPlayer::isRhythmChannel(int channelNr) const
{
	return rhythmMode && channelNr >= 6;
}

static inline int getSlotRegister(int base, int slotNr)
{
	return base + slotRegisterOffsets[slotNr];
}

void CcmfmacsoperaPlayer::setSlot(int slotNr, const SlotSettings& settings)
{
	opl->write(getSlotRegister(0x20, slotNr), (settings.multiple & 0xf) | ((settings.ksr & 1) << 4) | ((settings.egType & 1) << 5) | ((settings.vib & 1) << 6) | ((settings.ampMod & 1) << 7));
	opl->write(getSlotRegister(0x60, slotNr), ((settings.attackRate & 0xf) << 4) | (settings.decayRate & 0xf));
	opl->write(getSlotRegister(0x80, slotNr), ((settings.sustainLevel & 0xf) << 4) | (settings.releaseRate & 0xf));
	opl->write(getSlotRegister(0xe0, slotNr), settings.waveSelect & 3);
}

bool CcmfmacsoperaPlayer::setInstrument(int channelNr, const Instrument& inst)
{
	if (!isValidChannel(channelNr))
		return false;

	if (channelCurrentInstrument[channelNr] != &inst) {
		if (!isRhythmChannel(channelNr) || channelNr == CHANNEL_RHY_BD) {
			// normal 2-op channel (or BD in rhythm mode, which also has two slots)
			opl->write(0xc0 + channelNr, ((inst.feedback & 7) << 1) | (1 - (inst.connection & 1)));
			setSlot(channelSlots[channelNr].slotOp1, inst.op[0]);
			setSlot(channelSlots[channelNr].slotOp2, inst.op[1]);
		}
		else {
			// rhythm channel
			setSlot(channelSlotsRhythm[channelNr], inst.op[0]);
		}

		channelCurrentInstrument[channelNr] = &inst;
	}

	return true;
}

static int calculateAttenuation(int attenuation, int volumeColumn)
{
	if (attenuation  < 0  ) attenuation  = 0;
	if (attenuation  > 63 ) attenuation  = 63;
	if (volumeColumn < 0  ) volumeColumn = 0;
	if (volumeColumn > 127) volumeColumn = 127;
	
	// Instruments have a configured attenuation.
	// The "rest" of the attenuation (max. 63, 6 bits in "total level") is scaled by volumeColumn.
	// (This seems right for most values I've tested. But with att=8/vol=115, we get 13 here while SOUNDFX gets 14.
	return attenuation + ((63 - attenuation) * (127 - volumeColumn)) / 127;
}

void CcmfmacsoperaPlayer::keyOn(int channelNr)
{
	if (!isValidChannel(channelNr))
		return;

	if (!isRhythmChannel(channelNr)) {
		current0xBx[channelNr] |= (1 << 5);
		opl->write(0xb0 + channelNr, current0xBx[channelNr]);
	}
	else {
		// Channel 6 (BD) -> D4 .. Channel 10 (HH) -> D0
		current0xBD |= 1 << (10 - channelNr);
		opl->write(0xbd, current0xBD);
	}
}

void CcmfmacsoperaPlayer::keyOff(int channelNr)
{
	if (!isValidChannel(channelNr))
		return;

	if (!isRhythmChannel(channelNr)) {
		current0xBx[channelNr] &= ~(1 << 5);
		opl->write(0xb0 + channelNr, current0xBx[channelNr]);
	}
	else {
		// Channel 6 (BD) -> D4 .. Channel 10 (HH) -> D0
		current0xBD &= ~(1 << (10 - channelNr));
		opl->write(0xbd, current0xBD);
	}
}

void CcmfmacsoperaPlayer::setAxBx(int channelNr, int Ax, int Bx)
{
	if (channelNr < 0 || channelNr >= 8)
		return;

	opl->write(0xa0 + channelNr, Ax);
	current0xBx[channelNr] = Bx;
	opl->write(0xb0 + channelNr, current0xBx[channelNr]);
}

bool CcmfmacsoperaPlayer::setNote(int channelNr, int note)
{
	if (!isValidChannel(channelNr))
		return false;

	if (note < 23 || note >= 120)
		return false;

	int fNumber = fNumbers[note % 12];

	int Ax = fNumber & 0xff;
	int Bx = ((fNumber >> 8) & 3) | ((note / 12 - 2) << 2);

	if (!isRhythmChannel(channelNr)) {
		setAxBx(channelNr, Ax, Bx);
	}
	else {
		// not entirely right, but this is what the original player seems to do
		if (channelNr == CHANNEL_RHY_BD)
			setAxBx(6, Ax, Bx);
		setAxBx(7, Ax, Bx);
		if (channelNr == CHANNEL_RHY_SN || channelNr == CHANNEL_RHY_TOM)
			setAxBx(8, Ax, Bx);
	}

	return true;
}

void CcmfmacsoperaPlayer::setVolume(int channelNr, int vol)
{
	if (!isValidChannel(channelNr) || !channelCurrentInstrument[channelNr])
		return;

	const Instrument& inst = *channelCurrentInstrument[channelNr];

	if (!isRhythmChannel(channelNr) || channelNr == CHANNEL_RHY_BD) {
		// normal 2-op channel (or BD in rhythm mode, which also has two slots)

		// Set volume on OP1: If OP1 modulates OP2 (inst.connection != 0), don't scale its volume by n.volume
		opl->write(
			getSlotRegister(0x40, channelSlots[channelNr].slotOp1),
			((inst.op[0].ksl & 3) << 6) |
			  (inst.connection == 0 ? calculateAttenuation(inst.op[0].totalLevel, vol) : (inst.op[0].totalLevel & 0x3f))
		);

		// Set volume on OP2
		opl->write(
			getSlotRegister(0x40, channelSlots[channelNr].slotOp2),
			((inst.op[1].ksl & 3) << 6) | calculateAttenuation(inst.op[1].totalLevel, vol)
		);
	}
	else {
		// rhythm channel
		opl->write(
			getSlotRegister(0x40, channelSlotsRhythm[channelNr]),
			((inst.op[1].ksl & 3) << 6) | calculateAttenuation(inst.op[0].totalLevel, vol)
		);
	}
}

bool CcmfmacsoperaPlayer::advanceRow()
{
	for (;;) {
		if (++currentRow >= 64) {
			// next pattern
			currentRow = 0;
			currentPatternIndex = 0;

			do {
				currentOrderIndex++; // overflows ~0 into 0 when needed

				// check bounds
				if (currentOrderIndex >= (sizeof(patternOrder) / sizeof(patternOrder[0])))
					return false;

				// end of song?
				if (patternOrder[currentOrderIndex] == 99)
					return false;

			} while (patternOrder[currentOrderIndex] >= patterns.size()); // loop to skip invalid pattern references

			AdPlug_LogWrite("order %u, pattern %d\n", currentOrderIndex, patternOrder[currentOrderIndex]);
		}

		// check for pattern break
		const Pattern &p = patterns[patternOrder[currentOrderIndex]];
		if (currentPatternIndex < p.size() && p[currentPatternIndex].row == currentRow && p[currentPatternIndex].note == 1) {
			currentRow = 64;
		}
		else // no pattern break, done!
			break;
	}

	return true;
}

void CcmfmacsoperaPlayer::processNoteEvent(const CcmfmacsoperaPlayer::NoteEvent &n)
{
	int channelNr = n.col;

	if (!isValidChannel(channelNr))
		return;

	keyOff(channelNr);

	if (n.note == 4) // key off: done here
		return;

	if (n.instrument >= 0 && n.instrument < instruments.size())
		setInstrument(channelNr, instruments[n.instrument]);

	setVolume(channelNr, n.volume);

	if (setNote(channelNr, n.note))
		keyOn(channelNr);
}

bool CcmfmacsoperaPlayer::update()
{
	AdPlug_LogWrite( "%2u: ", currentRow);

	const Pattern& p = patterns[patternOrder[currentOrderIndex]];

	int currentCol = 0;
	for (; currentPatternIndex < p.size() && p[currentPatternIndex].row == currentRow; ++currentPatternIndex) {
		const NoteEvent& n = p[currentPatternIndex];

		for (; currentCol < n.col; ++currentCol)
			AdPlug_LogWrite("             ");
		AdPlug_LogWrite( "%2d %2d %2x %2d  ", n.note, n.instrument, n.volume, n.pitch);

		currentCol++;
		processNoteEvent(n);
	}
	AdPlug_LogWrite("\n");

	bool stillPlaying = advanceRow();

	if (!stillPlaying) {
		resetPlayer();
		// Once the song is done, keep on returning false.
		// TODO: returning "false" once should suffice, but it doesn't (at least with adplay-unix&SDL)
		// "songDone" is only reset in rewind().
		songDone = true;
	}

	return !songDone;
}

void CcmfmacsoperaPlayer::resetPlayer()
{
	currentRow = 64;
	currentOrderIndex = ~0;
	advanceRow();
}

void CcmfmacsoperaPlayer::rewind(int subsong)
{
	opl->init();
	opl->write(1, 1 << 5);  // enable wave select

	// 0xbd:5 (RHY) enables "rhythm mode"
	current0xBD = rhythmMode ? (1 << 5) : 0;
	opl->write(0xbd, current0xBD);

	memset(current0xBx, 0, sizeof(current0xBx));
	memset(channelCurrentInstrument, 0, sizeof(channelCurrentInstrument));

	// This isn't right for all channels.
	// No songs I've seen rely on the undocumented "default instrument", so ...
	static const Instrument defaultInstrument = {
		{{0, 1, 15, 5, 0, 1, 0, 0, 0, 0, 0, 0},
		 {0, 1, 15, 7, 0, 2, 0, 0, 0, 0, 1, 0}},
		3, 0, {'D', 'e', 'f', 'a', 'u', 'l', 't', 0, 0, 0, 0, 0, 0, 0 }
	};
	for (int i = 0; i < 11; ++i)
		setInstrument(i, defaultInstrument);

	// see comment in update() why this is needed
	songDone = false;

	resetPlayer();
}
