/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2009 Simon Peter, <dn.tlp@gmx.net>, et al.
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
 * cmf.h - CMF player by Adam Nielsen <malvineous@shikadi.net>
 */

#include <stdint.h> // for uintxx_t
#include "player.h"

typedef struct {
	uint16_t iInstrumentBlockOffset;
	uint16_t iMusicOffset;
	uint16_t iTicksPerQuarterNote;
	uint16_t iTicksPerSecond;
	uint16_t iTagOffsetTitle;
	uint16_t iTagOffsetComposer;
	uint16_t iTagOffsetRemarks;
	uint8_t iChannelsInUse[16];
	uint16_t iNumInstruments;
	uint16_t iTempo;
} CMFHEADER;

typedef struct {
	uint8_t iCharMult;
	uint8_t iScalingOutput;
	uint8_t iAttackDecay;
	uint8_t iSustainRelease;
	uint8_t iWaveSel;
} OPERATOR;

typedef struct {
	OPERATOR op[2]; // 0 == modulator, 1 == carrier
	uint8_t iConnection;
} SBI;

typedef struct {
	int iPatch; // MIDI patch for this channel
	int iPitchbend; // Current pitchbend amount for this channel
	int iTranspose; // Transpose amount for this channel (between -128 and +128)
} MIDICHANNEL;

typedef struct {
	int iNoteStart;   // When the note started playing (longest notes get cut first, 0 == channel free)
	int iMIDINote;    // MIDI note number currently being played on this OPL channel
	int iMIDIChannel; // Source MIDI channel where this note came from
	int iMIDIPatch;   // Current MIDI patch set on this OPL channel
} OPLCHANNEL;

class CcmfPlayer: public CPlayer
{
	private:
		uint8_t *data; // song data (CMF music block)
		int iPlayPointer;		// Current location of playback pointer
		int iSongLen;       // Max value for iPlayPointer
		CMFHEADER cmfHeader;
		SBI *pInstruments;
		bool bPercussive; // are rhythm-mode instruments enabled?
		uint8_t iCurrentRegs[256]; // Current values in the OPL chip
		uint8_t iPrevCommand; // Previous command (used for repeated MIDI commands, as the seek and playback code need to share this)
		uint8_t iNotePlaying[16]; // Last note turned on, used for duplicate note check
		bool bNoteFix[16]; // Fix duplicated Note On / Note Off

		int iNoteCount;  // Used to count how long notes have been playing for
		MIDICHANNEL chMIDI[16];
		OPLCHANNEL chOPL[9];

		// Additions for AdPlug's design
		int iDelayRemaining;
		bool bSongEnd;
		std::string strTitle, strComposer, strRemarks;

	public:
		static CPlayer *factory(Copl *newopl);

		CcmfPlayer(Copl *newopl);
		~CcmfPlayer();

		bool load(const std::string &filename, const CFileProvider &fp);
		bool update();
		void rewind(int subsong);
		float getrefresh();

		std::string gettype()
			{ return std::string("Creative Music File (CMF)"); };
		std::string gettitle();
		std::string getauthor();
		std::string getdesc();

	protected:
		uint32_t readMIDINumber();
		void writeInstrumentSettings(uint8_t iChannel, uint8_t iOperatorSource, uint8_t iOperatorDest, uint8_t iInstrument);
		void writeOPL(uint8_t iRegister, uint8_t iValue);
		void getFreq(uint8_t iChannel, uint8_t iNote, uint8_t * iBlock, uint16_t * iOPLFNum);
		void cmfNoteOn(uint8_t iChannel, uint8_t iNote, uint8_t iVelocity);
		void cmfNoteOff(uint8_t iChannel, uint8_t iNote, uint8_t iVelocity);
		void cmfNoteUpdate(uint8_t iChannel);
		uint8_t getPercChannel(uint8_t iChannel);
		void MIDIchangeInstrument(uint8_t iOPLChannel, uint8_t iMIDIChannel, uint8_t iNewInstrument);
		void MIDIcontroller(uint8_t iChannel, uint8_t iController, uint8_t iValue);

};
