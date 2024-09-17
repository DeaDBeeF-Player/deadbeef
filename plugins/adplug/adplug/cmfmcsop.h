/*
 * SoundFX Macs Opera CMF Player -- Copyright (c) 2017 Sebastian Kienzl <seb@knzl.de>
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

#ifndef H_ADPLUG_CMFMACSOPERAPLAYER
#define H_ADPLUG_CMFMACSOPERAPLAYER

#include <vector>
#include <stdint.h>
#include "player.h"

class CcmfmacsoperaPlayer: public CPlayer
{
public:
	static CPlayer* factory(Copl* newopl);

	CcmfmacsoperaPlayer(Copl* newopl);

	bool load(const std::string& filename, const CFileProvider& fp);
	bool update();
	void rewind(int subsong);
	float getrefresh() { return speedRowsPerSec; };

	virtual std::string gettype();
	virtual unsigned int getpatterns()    { return nrOfPatterns; }
	virtual unsigned int getpattern()     { return patternOrder[currentOrderIndex]; }
	virtual unsigned int getorders()      { return nrOfOrders; }
	virtual unsigned int getorder()       { return currentOrderIndex; }
	virtual unsigned int getrow()         { return currentRow; }
	virtual unsigned int getspeed()       { return 1; }
	virtual unsigned int getinstruments() { return instruments.size(); }
	virtual std::string getinstrument(unsigned int n) { return instruments[n].name; }

 protected:
	struct SlotSettings {
		int16_t ksl;
		int16_t multiple;
		int16_t attackRate;
		int16_t sustainLevel;
		int16_t egType;
		int16_t decayRate;
		int16_t releaseRate;
		int16_t totalLevel;
		int16_t ampMod;
		int16_t vib;
		int16_t ksr;
		int16_t waveSelect;
	};

	struct Instrument {
		SlotSettings op[2];
		int16_t      feedback;
		int16_t      connection;
		char         name[14];
	};

	struct NoteEvent {
		uint8_t row;
		uint8_t col;
		uint8_t note;    // 4: release, 1: end of pattern, 24: C-0, 119: H-7/B#7
		uint8_t instrument;
		uint8_t volume;
		uint8_t pitch;
	};
	
	typedef std::vector<NoteEvent> Pattern;

	float   speedRowsPerSec;
	bool    rhythmMode;
	bool    songDone;

	int      nrOfPatterns;
	uint16_t patternOrder[99];
	int      nrOfOrders;

	std::vector<Instrument> instruments;
	std::vector<Pattern> patterns;
	
	unsigned int currentOrderIndex;
	unsigned int currentRow;
	unsigned int currentPatternIndex;
	
    const Instrument* channelCurrentInstrument[11];
	int current0xBx[9];
	int current0xBD;


	bool loadInstruments(binistream* f, int nrOfInstruments);
	bool loadPatterns(binistream* f);

	bool isValidChannel(int channelNr) const;
	bool isRhythmChannel(int channelNr) const;

	void setSlot(int slotNr, const SlotSettings& settings);
	bool setInstrument(int channelNr, const Instrument& inst);
	void setAxBx(int channelNr, int Ax, int Bx);
	bool setNote(int channelNr, int note);
	void setVolume(int channelNr, int vol);

	void keyOn(int channelNr);
	void keyOff(int channelNr);

	bool advanceRow();
	void processNoteEvent(const NoteEvent &n);

	void resetPlayer();
};

#endif
