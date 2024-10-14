/*
* Adplug - Replayer for many OPL2/OPL3 audio file formats.
* Copyright (C) 1999 - 2007 Simon Peter, <dn.tlp@gmx.net>, et al.
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
* rad2.cpp - RAD v2 replayer class
*
* Aside from the Crad2Player wrapper class, all of the code in this file is based 
* directly on the public domain RAD2 replayer source by Shayde/Reality, updated
* to also support the original RAD v1 format. It is otherwise modified as little 
* as possible.
*/

#include "rad2.h"
#include "debug.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

/*

Code to check a RAD V2 tune file is valid.  That is, it will check the tune
can be played without crashing the player.  It doesn't exhaustively check
the tune except where needed to prevent a possible player crash.

Call RADValidate with a pointer to your tune and the size in bytes.  It
will return either NULL for all okay, or a pointer to a null-terminated
string describing what is wrong with the data.

*/


//==================================================================================================
// The error strings are all supplied here in case you want to translate them to another language
// (or supply your own more descriptive error messages).
//==================================================================================================
static const char *g_RADNotATuneFile = "Not a RAD tune file.";
static const char *g_RADNotAVersion21Tune = "Not a version 1.0 or 2.1 file format RAD tune.";
static const char *g_RADTruncated = "Tune file has been truncated and is incomplete.";
static const char *g_RADBadFlags = "Tune file has invalid flags.";
static const char *g_RADBadBPMValue = "Tune's BPM value is out of range.";
static const char *g_RADBadInstrument = "Tune file contains a bad instrument definition.";
static const char *g_RADUnknownMIDIVersion = "Tune file contains an unknown MIDI instrument version.";
static const char *g_RADOrderListTooLarge = "Order list in tune file is an invalid size.";
static const char *g_RADBadJumpMarker = "Order list jump marker is invalid.";
static const char *g_RADBadOrderEntry = "Order list entry is invalid.";
static const char *g_RADBadPattNum = "Tune file contains a bad pattern index.";
static const char *g_RADPattTruncated = "Tune file contains a truncated pattern.";
static const char *g_RADPattExtraData = "Tune file contains a pattern with extraneous data.";
static const char *g_RADPattBadLineNum = "Tune file contains a pattern with a bad line definition.";
static const char *g_RADPattBadChanNum = "Tune file contains a pattern with a bad channel definition.";
static const char *g_RADPattBadNoteNum = "Pattern contains a bad note number.";
static const char *g_RADPattBadInstNum = "Pattern contains a bad instrument number.";
static const char *g_RADPattBadEffect = "Pattern contains a bad effect and/or parameter.";
static const char *g_RADBadRiffNum = "Tune file contains a bad riff index.";
//static const char *g_RADExtraBytes = "Tune file contains extra bytes.";



//==================================================================================================
// Validate a RAD V2 (file format 2.1) tune file.  Note, this uses no C++ standard library code.
//==================================================================================================
static const char *RADCheckPattern(const uint8_t *&s, const uint8_t *e, bool riff) {

	// Get pattern size
	if (s + 2 > e)
		return g_RADTruncated;
	uint16_t pattsize = s[0] | (uint16_t(s[1]) << 8);
	s += 2;

	// Calculate end of pattern
	const uint8_t *pe = s + pattsize;
	if (pe > e)
		return g_RADTruncated;

	uint8_t linedef, chandef;
	do {

		// Check line of pattern
		if (s >= pe)
			return g_RADPattTruncated;
		linedef = *s++;
		uint8_t linenum = linedef & 0x7F;
		if (linenum >= 64)
			return g_RADPattBadLineNum;

		do {

			// Check channel of pattern
			if (s >= pe)
				return g_RADPattTruncated;
			chandef = *s++;
			uint8_t channum = chandef & 0x0F;
			if (!riff && channum >= 9)
				return g_RADPattBadChanNum;

			// Check note
			if (chandef & 0x40) {
				if (s >= pe)
					return g_RADPattTruncated;
				uint8_t note = *s++;
				uint8_t notenum = note & 15;
				//uint8_t octave = (note >> 4) & 7;
				if (notenum == 0 || notenum == 13 || notenum == 14)
					return g_RADPattBadNoteNum;
			}

			// Check instrument.  This shouldn't be supplied if bit 7 of the note byte is set,
			// but it doesn't break anything if it is so we don't check for it
			if (chandef & 0x20) {
				if (s >= pe)
					return g_RADPattTruncated;
				uint8_t inst = *s++;
				if (inst == 0 || inst >= 128)
					return g_RADPattBadInstNum;
			}

			// Check effect.  A non-existent effect could be supplied, but it'll just be
			// ignored by the player so we don't care
			if (chandef & 0x10) {
				if (s + 2 > pe)
					return g_RADPattTruncated;
				uint8_t effect = *s++;
				uint8_t param = *s++;
				if (effect > 31 || param > 99)
					return g_RADPattBadEffect;
			}

		} while (!(chandef & 0x80));

	} while (!(linedef & 0x80));

	if (s != pe)
		return g_RADPattExtraData;

	return 0;
}
//--------------------------------------------------------------------------------------------------
static const char *RADCheckPatternOld(const uint8_t *&s, const uint8_t *e) {

	if (s > e)
		return g_RADTruncated;

	uint8_t linedef, chandef;
	do {

		// Check line of pattern
		if (s >= e)
			return g_RADPattTruncated;
		linedef = *s++;
		uint8_t linenum = linedef & 0x7F;
		if (linenum >= 64)
			return g_RADPattBadLineNum;

		do {

			// Check channel of pattern
			if (s >= e)
				return g_RADPattTruncated;
			chandef = *s++;
			uint8_t channum = chandef & 0x0F;
			if (channum >= 9)
				return g_RADPattBadChanNum;

			// Check note
			if (s >= e)
				return g_RADPattTruncated;
			/*uint8_t note = *s++; */ s++;
			//uint8_t notenum = note & 15;
			//uint8_t octave = (note >> 4) & 7;
			/* the replayer handles bad params already and some old tunes do contain them
			if (notenum == 13 || notenum == 14)
				return g_RADPattBadNoteNum;
			*/

			// Check instrument
			if (s >= e)
				return g_RADPattTruncated;
			uint8_t inst = *s++;

			// Check effect
			if (inst & 0xf) {
				if (s > e)
					return g_RADPattTruncated;
				/* uint8_t param = *s++; */ s++;
				/* the replayer handles bad params already and some old tunes do contain them
				if (param > 99)
					return g_RADPattBadEffect;
				*/
			}

		} while (!(chandef & 0x80));

	} while (!(linedef & 0x80));

	return 0;
}
//--------------------------------------------------------------------------------------------------
static const char *RADValidate(const void *data, size_t data_size) {

	const uint8_t *s = (const uint8_t *)data;
	const uint8_t *e = s + data_size;

	int Version;

	// Check header
	if (data_size < 16)
		return g_RADNotATuneFile;

	s += 16;

	// Check version
	if (s >= e || (*s != 0x10 && *s != 0x21))
		return g_RADNotAVersion21Tune;
	Version = (*s) >> 4;
	s++;

	// Check flags
	if (s >= e)
		return g_RADTruncated;

	uint8_t flags = *s++;
	if (Version >= 2 && flags & 0x80)
		return g_RADBadFlags; // Bit 7 is unused in v2
	if (Version == 1 && flags & 0x20)
		return g_RADBadFlags; // Bit 5 is unused in v1

	if (Version >= 2 && flags & 0x20) {
		if (s + 2 > e)
			return g_RADTruncated;
		uint16_t bpm = s[0] | (uint16_t(s[1]) << 8);
		s += 2;
		if (bpm < 46 || bpm > 300)
			return g_RADBadBPMValue;
	}

	// Check description.  This is actually freeform text so there's not a lot to check, just that
	// it's a null-terminated string
	if (Version >= 2 || flags & 0x80) {
		do {
			if (s >= e)
				return g_RADTruncated;
		} while (*s++);
	}

	// Check instruments.  We don't actually validate the individual instrument fields as the tune
	// file will still play with bad instrument data.  We're only concerned that the tune file
	// doesn't crash the player
	uint8_t last_inst = 0;
	while (1) {

		// Get instrument number, or 0 for end of instrument list
		if (s >= e)
			return g_RADTruncated;
		uint8_t inst = *s++;
		if (inst == 0)
			break;

		// RAD always saves the instruments out in order
		if (inst > 127 || inst <= last_inst)
			return g_RADBadInstrument;
		last_inst = inst;

		if (Version >= 2) {
			// Check the name
			if (s >= e)
				return g_RADTruncated;
			uint8_t namelen = *s++;
			s += namelen;

			// Get algorithm
			if (s > e)
				return g_RADTruncated;
			uint8_t alg = *s;

			if ((alg & 7) == 7) {

				// MIDI instrument.  We need to check the version as this can affect the following
				// data size
				if (s + 7 > e)
					return g_RADTruncated;
				if (s[2] >> 4)
					return g_RADUnknownMIDIVersion;
				s += 7;

			}
			else {

				s += 24;
				if (s > e)
					return g_RADTruncated;
			}

			// Riff track supplied?
			if (alg & 0x80) {

				const char *err = RADCheckPattern(s, e, false);
				if (err)
					return err;
			}
		}
		else {
			// skip version 1 instruments
			s += 11;
		}

	}

	// Get the order list
	if (s >= e)
		return g_RADTruncated;
	uint8_t order_size = *s++;
	const uint8_t *order_list = s;
	if (order_size > 128)
		return g_RADOrderListTooLarge;
	s += order_size;

	for (uint8_t i = 0; i < order_size; i++) {
		uint8_t order = order_list[i];

		if (order & 0x80) {

			// Check jump marker
			order &= 0x7F;
			if (order >= order_size)
				return g_RADBadJumpMarker;
		}
		else {

			// Check pattern number.  It doesn't matter if there is no pattern with this number
			// defined later, as missing patterns are treated as empty
			if (Version >= 2 && order >= 100)
				return g_RADBadOrderEntry;
			if (Version == 1 && order >= 32)
				return g_RADBadOrderEntry;
		}
	}

	// Check the patterns
	if (Version >= 2) while (1) {
		// Version 2 patterns
		// Get pattern number
		if (s >= e)
			return g_RADTruncated;
		uint8_t pattnum = *s++;

		// Last pattern?
		if (pattnum == 0xFF)
			break;

		if (pattnum >= 100)
			return g_RADBadPattNum;

		const char *err = RADCheckPattern(s, e, false);
		if (err)
			return err;
	}
	else for (int i = 0; i < 32; i++) {
		// Version 1 patterns
		if (s + 2 > e)
			return g_RADTruncated;

		int pos = s[0] | (int(s[1]) << 8);
		s += 2;
		if (pos) {
			const uint8_t *patt = (uint8_t*)data + pos;
			const char *err = RADCheckPatternOld(patt, e);
			if (err)
				return err;
		}
	}

	// Check the riffs
	if (Version >= 2) while (1) {

		// Get riff number
		if (s >= e)
			return g_RADTruncated;
		uint8_t riffnum = *s++;

		// Last riff?
		if (riffnum == 0xFF)
			break;

		uint8_t riffpatt = riffnum >> 4;
		uint8_t riffchan = riffnum & 15;
		if (riffpatt > 9 || riffchan == 0 || riffchan > 9)
			return g_RADBadRiffNum;

		const char *err = RADCheckPattern(s, e, true);
		if (err)
			return err;
	}

	// Tune file is all good
	return 0;
}

/*

C++ player code for Reality Adlib Tracker 2.0a (file version 2.1).

Please note, this is just the player code.  This does no checking of the tune data before
it tries to play it, as most use cases will be a known tune being used in a production.
So if you're writing an application that loads unknown tunes in at run time then you'll
want to do more validity checking.

To use:

- Instantiate the RADPlayer object

- Initialise player for your tune by calling the Init() method.  Supply a pointer to the
tune file and a function for writing to the OPL3 registers.

- Call the Update() method a number of times per second as returned by GetHertz().  If
your tune is using the default BPM setting you can safely just call it 50 times a
second, unless it's a legacy "slow-timer" tune then it'll need to be 18.2 times a
second.

- When you're done, stop calling Update() and call the Stop() method to turn off all
sound and reset the OPL3 hardware.

*/

#ifndef RAD_DETECT_REPEATS
#define RAD_DETECT_REPEATS  1
#endif

//==================================================================================================
// RAD player class.
//==================================================================================================
class RADPlayer {

	// Various constants
	enum {
		kTracks = 100,
		kChannels = 9,
		kTrackLines = 64,
		kRiffTracks = 10,
		kInstruments = 127,

		cmPortamentoUp = 0x1,
		cmPortamentoDwn = 0x2,
		cmToneSlide = 0x3,
		cmToneVolSlide = 0x5,
		cmVolSlide = 0xA,
		cmSetVol = 0xC,
		cmJumpToLine = 0xD,
		cmSetSpeed = 0xF,
		cmIgnore = ('I' - 55),
		cmMultiplier = ('M' - 55),
		cmRiff = ('R' - 55),
		cmTranspose = ('T' - 55),
		cmFeedback = ('U' - 55),
		cmVolume = ('V' - 55),
	};

	enum e_Source {
		SNone, SRiff, SIRiff,
	};

	enum {
		fKeyOn = 1 << 0,
		fKeyOff = 1 << 1,
		fKeyedOn = 1 << 2,
	};

	struct CInstrument {
		uint8_t             Feedback[2];
		uint8_t             Panning[2];
		uint8_t             Algorithm;
		uint8_t             Detune;
		uint8_t             Volume;
		uint8_t             RiffSpeed;
		uint8_t *           Riff;
		uint8_t             Operators[4][5];
		char                Name[256];
	};

	struct CEffects {
		int8_t              PortSlide;
		int8_t              VolSlide;
		uint16_t            ToneSlideFreq;
		uint8_t             ToneSlideOct;
		uint8_t             ToneSlideSpeed;
		int8_t              ToneSlideDir;
	};

	struct CChannel {
		uint8_t             LastInstrument;
		CInstrument *       Instrument;
		uint8_t             Volume;
		uint8_t             DetuneA;
		uint8_t             DetuneB;
		uint8_t             KeyFlags;
		uint16_t            CurrFreq;
		int8_t              CurrOctave;
		CEffects            FX;
		struct CRiff {
			CEffects        FX;
			uint8_t *       Track;
			uint8_t *       TrackStart;
			uint8_t         Line;
			uint8_t         Speed;
			uint8_t         SpeedCnt;
			int8_t          TransposeOctave;
			int8_t          TransposeNote;
			uint8_t         LastInstrument;
		} Riff, IRiff;
	};

public:
	RADPlayer() : Initialised(false) {}
	void                Init(const void *tune, void(*opl3)(void *, uint16_t, uint8_t), void *arg);
	void                Stop();
	bool                Update();
	int                 GetVersion() const { return Version; }
	const uint8_t*      GetDescription() const { return Description; }
	float               GetHertz() const { return Hertz; }
	int                 GetPlayTimeInSeconds() const { return PlayTime / Hertz; }
	int                 GetTunePos() const { return Order; }
	int                 GetTuneLength() const { return OrderListSize; }
	int                 GetTunePattern() const { return OrderList[Order]; }
	int                 GetTunePatterns() const { return NumTracks; }
	int                 GetTuneLine() const { return Line; }
	const char*         GetTuneInst(uint8_t inst) const { return Instruments[inst].Name; }
	int                 GetTuneInsts() const { return NumInstruments; }
	void                SetMasterVolume(int vol) { MasterVol = vol; }
	int                 GetMasterVolume() const { return MasterVol; }
	int                 GetSpeed() const { return Speed; }

#if RAD_DETECT_REPEATS
	uint32_t            ComputeTotalTime();
#endif

private:
	bool                UnpackNote(uint8_t *&s, uint8_t &last_instrument);
	uint8_t *           GetTrack();
	uint8_t *           SkipToLine(uint8_t *trk, uint8_t linenum, bool chan_riff = false);
	void                PlayLine();
	void                PlayNote(int channum, int8_t notenum, int8_t octave, uint16_t instnum, uint8_t cmd = 0, uint8_t param = 0, e_Source src = SNone, int op = 0);
	void                LoadInstrumentOPL3(int channum);
	void                PlayNoteOPL3(int channum, int8_t octave, int8_t note);
	void                ResetFX(CEffects *fx);
	void                TickRiff(int channum, CChannel::CRiff &riff, bool chan_riff);
	void                ContinueFX(int channum, CEffects *fx);
	void                SetVolume(int channum, uint8_t vol);
	void                GetSlideDir(int channum, CEffects *fx);
	void                LoadInstMultiplierOPL3(int channum, int op, uint8_t mult);
	void                LoadInstVolumeOPL3(int channum, int op, uint8_t vol);
	void                LoadInstFeedbackOPL3(int channum, int which, uint8_t fb);
	void                Portamento(uint16_t channum, CEffects *fx, int8_t amount, bool toneslide);
	void                Transpose(int8_t note, int8_t octave);
	void                SetOPL3(uint16_t reg, uint8_t val) {
		OPL3Regs[reg] = val;
		OPL3(OPL3Arg, reg, val);
	}
	uint8_t             GetOPL3(uint16_t reg) const {
		return OPL3Regs[reg];
	}

	void(*OPL3)(void *, uint16_t, uint8_t);
	void *              OPL3Arg;
	int                 Version;
	bool                UseOPL3;
	const uint8_t*      Description;
	CInstrument         Instruments[kInstruments];
	int                 NumInstruments;
	CChannel            Channels[kChannels];
	uint32_t            PlayTime;
#if RAD_DETECT_REPEATS
	uint32_t            OrderMap[4];
	bool                Repeating;
#endif
	float               Hertz;
	uint8_t *           OrderList;
	uint8_t *           Tracks[kTracks];
	int                 NumTracks;
	uint8_t *           Riffs[kRiffTracks][kChannels];
	uint8_t *           Track;
	bool                Initialised;
	uint8_t             Speed;
	uint8_t             OrderListSize;
	uint8_t             SpeedCnt;
	uint8_t             Order;
	uint8_t             Line;
	int8_t              Entrances;
	uint8_t             MasterVol;
	int8_t              LineJump;
	uint8_t             OPL3Regs[512];

	// Values exported by UnpackNote()
	int8_t              NoteNum;
	int8_t              OctaveNum;
	uint8_t             InstNum;
	uint8_t             EffectNum;
	uint8_t             Param;
	bool                LastNote;

	static const int8_t NoteSize[];
	static const uint16_t ChanOffsets3[9], Chn2Offsets3[9];
	static const uint16_t NoteFreq[];
	static const uint16_t OpOffsets2[9][2];
	static const uint16_t OpOffsets3[9][4];
	static const bool   AlgCarriers[7][4];
};
//--------------------------------------------------------------------------------------------------
const int8_t RADPlayer::NoteSize[] = { 0, 2, 1, 3, 1, 3, 2, 4 };
const uint16_t RADPlayer::ChanOffsets3[9] = { 0, 1, 2, 0x100, 0x101, 0x102, 6, 7, 8 };              // OPL3 first channel
const uint16_t RADPlayer::Chn2Offsets3[9] = { 3, 4, 5, 0x103, 0x104, 0x105, 0x106, 0x107, 0x108 };  // OPL3 second channel
const uint16_t RADPlayer::NoteFreq[] = { 0x16b,0x181,0x198,0x1b0,0x1ca,0x1e5,0x202,0x220,0x241,0x263,0x287,0x2ae };
const uint16_t RADPlayer::OpOffsets2[9][2] = {
	{ 0x003, 0x000 },
	{ 0x004, 0x001 },
	{ 0x005, 0x002 },
	{ 0x00B, 0x008 },
	{ 0x00C, 0x009 },
	{ 0x00D, 0x00A },
	{ 0x013, 0x010 },
	{ 0x014, 0x011 },
	{ 0x015, 0x012 }
};
const uint16_t RADPlayer::OpOffsets3[9][4] = {
	{ 0x00B, 0x008, 0x003, 0x000 },
	{ 0x00C, 0x009, 0x004, 0x001 },
	{ 0x00D, 0x00A, 0x005, 0x002 },
	{ 0x10B, 0x108, 0x103, 0x100 },
	{ 0x10C, 0x109, 0x104, 0x101 },
	{ 0x10D, 0x10A, 0x105, 0x102 },
	{ 0x113, 0x110, 0x013, 0x010 },
	{ 0x114, 0x111, 0x014, 0x011 },
	{ 0x115, 0x112, 0x015, 0x012 }
};
const bool RADPlayer::AlgCarriers[7][4] = {
	{ true, false, false, false },  // 0 - 2op - op < op
	{ true, true,  false, false },  // 1 - 2op - op + op
	{ true, false, false, false },  // 2 - 4op - op < op < op < op
	{ true, false, false, true },  // 3 - 4op - op < op < op + op
	{ true, false, true,  false },  // 4 - 4op - op < op + op < op
	{ true, false, true,  true },  // 5 - 4op - op < op + op + op
	{ true, true,  true,  true },  // 6 - 4op - op + op + op + op
};



//==================================================================================================
// Initialise a RAD tune for playback.  This assumes the tune data is valid and does minimal data
// checking.
//==================================================================================================
void RADPlayer::Init(const void *tune, void(*opl3)(void *, uint16_t, uint8_t), void *arg) {

	Initialised = false;

	// Version check; we only support version 1.0 and 2.1 tune files
	uint8_t ver = *((uint8_t *)tune + 0x10);
	if (ver != 0x10 && ver != 0x21) {
		Hertz = -1;
		return;
	}
	Version = ver >> 4;
	UseOPL3 = Version >= 2;

	// The OPL3 call-back
	OPL3 = opl3;
	OPL3Arg = arg;

	for (int i = 0; i < kTracks; i++)
		Tracks[i] = 0;

	for (int i = 0; i < kRiffTracks; i++)
		for (int j = 0; j < kChannels; j++)
			Riffs[i][j] = 0;

	uint8_t *s = (uint8_t *)tune + 0x11;

	uint8_t flags = *s++;
	Speed = flags & 0x1F;

	// Is BPM value present?
	Hertz = 50.0f;
	if (Version >= 2 && flags & 0x20) {
		Hertz = (float)(s[0] | (int(s[1]) << 8)) * 2 / 5;
		s += 2;
	}

	// Slow timer tune?  Return an approximate hz
	if (flags & 0x40)
		Hertz = 18.2f;

	// Skip any description
	Description = 0;
	if (Version >= 2 || flags & 0x80) {
		Description = s;
		while (*s)
			s++;
		s++;
	}

	// Blank the instrument table, some files reference non-existent instruments
	memset(&Instruments[0], 0, sizeof(Instruments));

	// Unpack the instruments
	NumInstruments = 0;
	while (1) {

		// Instrument number, 0 indicates end of list
		uint8_t inst_num = *s++;
		if (inst_num == 0)
			break;
		if (inst_num  > NumInstruments)
			NumInstruments = inst_num;

		CInstrument &inst = Instruments[inst_num - 1];

		if (Version >= 2) {
			// Version 2 instrument
			// Read instrument name
			uint8_t inst_namelen = *s++;
			for (int i = 0; i < inst_namelen; i++)
				inst.Name[i] = *s++;
			inst.Name[inst_namelen] = '\0';

			uint8_t alg = *s++;
			inst.Algorithm = alg & 7;
			inst.Panning[0] = (alg >> 3) & 3;
			inst.Panning[1] = (alg >> 5) & 3;

			if (inst.Algorithm < 7) {

				uint8_t b = *s++;
				inst.Feedback[0] = b & 15;
				inst.Feedback[1] = b >> 4;

				b = *s++;
				inst.Detune = b >> 4;
				inst.RiffSpeed = b & 15;

				inst.Volume = *s++;

				for (int i = 0; i < 4; i++) {
					uint8_t *op = inst.Operators[i];
					for (int j = 0; j < 5; j++)
						op[j] = *s++;
				}

			}
			else {

				// Ignore MIDI instrument data
				s += 6;
			}

			// Instrument riff?
			if (alg & 0x80) {
				int size = s[0] | (int(s[1]) << 8);
				s += 2;
				inst.Riff = s;
				s += size;
			}
			else
				inst.Riff = 0;
		}
		else {
			// Version 1 instrument
			inst.Name[0] = '\0';

			inst.Algorithm = s[8] & 1;
			inst.Panning[0] = 0;
			inst.Panning[1] = 0;

			inst.Feedback[0] = (s[8] >> 1) & 0x7;
			inst.Feedback[1] = 0;

			inst.Detune = 0;
			inst.RiffSpeed = 0;

			inst.Volume = 64;

			inst.Operators[0][0] = s[0];
			inst.Operators[1][0] = s[1];
			inst.Operators[2][0] = 0;
			inst.Operators[3][0] = 0;
			inst.Operators[0][1] = s[2];
			inst.Operators[1][1] = s[3];
			inst.Operators[2][1] = 0;
			inst.Operators[3][1] = 0;
			inst.Operators[0][2] = s[4];
			inst.Operators[1][2] = s[5];
			inst.Operators[2][2] = 0;
			inst.Operators[3][2] = 0;
			inst.Operators[0][3] = s[6];
			inst.Operators[1][3] = s[7];
			inst.Operators[2][3] = 0;
			inst.Operators[3][3] = 0;
			inst.Operators[0][4] = s[9];
			inst.Operators[1][4] = s[10];
			inst.Operators[2][4] = 0;
			inst.Operators[3][4] = 0;

			inst.Riff = 0;

			s += 11;
		}
	}

	// Get order list
	OrderListSize = *s++;
	OrderList = s;
	s += OrderListSize;

	// Locate the tracks
	NumTracks = 0;
	if (Version >= 2) while (1) {
		// Version 2 tracks
		// Track number
		uint8_t track_num = *s++;
		if (track_num >= kTracks)
			break;
		if (track_num + 1 > NumTracks)
			NumTracks = track_num + 1;

		// Track size in bytes
		int size = s[0] | (int(s[1]) << 8);
		s += 2;

		Tracks[track_num] = s;
		s += size;
	}
	else for (int i = 0; i < 32; i++) {
		// Version 1 tracks
		int pos = s[0] | (int(s[1]) << 8);
		s += 2;
		if (pos) {
			NumTracks = i + 1;
			Tracks[i] = (uint8_t*)tune + pos;
		}
	}

	// Locate the riffs
	if (Version >= 2) while (1) {

		// Riff id
		uint8_t riffid = *s++;
		uint8_t riffnum = riffid >> 4;
		uint8_t channum = riffid & 15;
		if (riffnum >= kRiffTracks || channum > kChannels)
			break;

		// Track size in bytes
		int size = s[0] | (int(s[1]) << 8);
		s += 2;

		Riffs[riffnum][channum - 1] = s;
		s += size;
	}

	// Done parsing tune, now set up for play
	for (int i = 0; i < 512; i++)
		OPL3Regs[i] = 255;
	Stop();

	Initialised = true;
}



//==================================================================================================
// Stop all sounds and reset the tune.  Tune will play from the beginning again if you continue to
// Update().
//==================================================================================================
void RADPlayer::Stop() {

	// Clear all registers
	for (uint16_t reg = 0x20; reg < 0xF6; reg++) {

		// Ensure envelopes decay all the way
		uint8_t val = (reg >= 0x60 && reg < 0xA0) ? 0xFF : 0;

		SetOPL3(reg, val);
		SetOPL3(reg + 0x100, val);
	}

	// Configure OPL3
	SetOPL3(1, 0x20);  // Allow waveforms
	SetOPL3(8, 0);     // No split point
	SetOPL3(0xbd, 0);  // No drums, etc.
	SetOPL3(0x104, 0); // Everything 2-op by default
	SetOPL3(0x105, 1); // OPL3 mode on

#if RAD_DETECT_REPEATS
					   // The order map keeps track of which patterns we've played so we can detect when the tune
					   // starts to repeat.  Jump markers can't be reliably used for this
	PlayTime = 0;
	Repeating = false;
	for (int i = 0; i < 4; i++)
		OrderMap[i] = 0;
#endif

	// Initialise play values
	SpeedCnt = 1;
	Order = 0;
	Track = GetTrack();
	Line = 0;
	Entrances = 0;
	MasterVol = 64;

	// Initialise channels
	for (int i = 0; i < kChannels; i++) {
		CChannel &chan = Channels[i];
		chan.LastInstrument = 0;
		chan.Instrument = 0;
		chan.Volume = 0;
		chan.DetuneA = 0;
		chan.DetuneB = 0;
		chan.KeyFlags = 0;
		chan.Riff.SpeedCnt = 0;
		chan.IRiff.SpeedCnt = 0;
	}
}



//==================================================================================================
// Playback update.  Call BPM * 2 / 5 times a second.  Use GetHertz() for this number after the
// tune has been initialised.  Returns true if tune is starting to repeat.
//==================================================================================================
bool RADPlayer::Update() {

	if (!Initialised)
		return false;

	// Run riffs
	for (int i = 0; i < kChannels; i++) {
		CChannel &chan = Channels[i];
		TickRiff(i, chan.IRiff, false);
		TickRiff(i, chan.Riff, true);
	}

	// Run main track
	PlayLine();

	// Run effects
	for (int i = 0; i < kChannels; i++) {
		CChannel &chan = Channels[i];
		ContinueFX(i, &chan.IRiff.FX);
		ContinueFX(i, &chan.Riff.FX);
		ContinueFX(i, &chan.FX);
	}

	// Update play time.  We convert to seconds when queried
	PlayTime++;

#if RAD_DETECT_REPEATS
	return Repeating;
#else
	return false;
#endif
}



//==================================================================================================
// Unpacks a single RAD note.
//==================================================================================================
bool RADPlayer::UnpackNote(uint8_t *&s, uint8_t &last_instrument) {

	uint8_t chanid = *s++;

	InstNum = 0;
	EffectNum = 0;
	Param = 0;

	uint8_t note = 0;

	if (Version >= 2) {
		// Version 2 notes
		// Unpack note data
		if (chanid & 0x40) {
			uint8_t n = *s++;
			note = n & 0x7F;

			// Retrigger last instrument?
			if (n & 0x80)
				InstNum = last_instrument;
		}

		// Do we have an instrument?
		if (chanid & 0x20) {
			InstNum = *s++;
			last_instrument = InstNum;
		}

		// Do we have an effect?
		if (chanid & 0x10) {
			EffectNum = *s++;
			Param = *s++;
		}
	}
	else {
		// Version 1 notes
		// Unpack note data
		uint8_t n = *s++;
		note = n & 0x7f;
		if (n & 0x80)
			InstNum = 16;

		// Do we have an instrument?
		n = *s++;
		InstNum |= n >> 4;
		if (InstNum)
			last_instrument = InstNum;

		// Do we have an effect?
		EffectNum = n & 0xf;
		if (EffectNum)
			Param = *s++;
	}

	NoteNum = note & 15;
	OctaveNum = note >> 4;

	return ((chanid & 0x80) != 0);
}



//==================================================================================================
// Get current track as indicated by order list.
//==================================================================================================
uint8_t *RADPlayer::GetTrack() {

	// If at end of tune start again from beginning
	if (Order >= OrderListSize)
		Order = 0;

	uint8_t track_num = OrderList[Order];

	// Jump marker?  Note, we don't recognise multiple jump markers as that could put us into an
	// infinite loop
	if (track_num & 0x80) {
		Order = track_num & 0x7F;
		track_num = OrderList[Order] & 0x7F;
	}

#if RAD_DETECT_REPEATS
	// Check for tune repeat, and mark order in order map
	if (Order < 128) {
		int byte = Order >> 5;
		uint32_t bit = uint32_t(1) << (Order & 31);
		if (OrderMap[byte] & bit)
			Repeating = true;
		else
			OrderMap[byte] |= bit;
	}
#endif

	return Tracks[track_num];
}



//==================================================================================================
// Skip through track till we reach the given line or the next higher one.  Returns null if none.
//==================================================================================================
uint8_t *RADPlayer::SkipToLine(uint8_t *trk, uint8_t linenum, bool chan_riff) {

	while (1) {

		uint8_t lineid = *trk;
		if ((lineid & 0x7F) >= linenum)
			return trk;
		if (lineid & 0x80)
			break;
		trk++;

		// Skip channel notes
		uint8_t chanid;
		do {
			chanid = *trk++;
			if (Version >= 2)
				trk += NoteSize[(chanid >> 4) & 7];
			else if (trk[1] & 0xf)
				// v1 note with param
				trk += 3;
			else
				// v1 note without param
				trk += 2;
		} while (!(chanid & 0x80) && !chan_riff);
	}

	return 0;
}



//==================================================================================================
// Plays one line of current track and advances pointers.
//==================================================================================================
void RADPlayer::PlayLine() {

	SpeedCnt--;
	if (SpeedCnt > 0)
		return;
	SpeedCnt = Speed;

	// Reset channel effects
	for (int i = 0; i < kChannels; i++)
		ResetFX(&Channels[i].FX);

	LineJump = -1;

	// At the right line?
	uint8_t *trk = Track;
	if (trk && (*trk & 0x7F) <= Line) {
		uint8_t lineid = *trk++;

		// Run through channels
		bool last;
		do {
			int channum = *trk & 15;
			CChannel &chan = Channels[channum];
			last = UnpackNote(trk, chan.LastInstrument);
			PlayNote(channum, NoteNum, OctaveNum, InstNum, EffectNum, Param);
		} while (!last);

		// Was this the last line?
		if (lineid & 0x80)
			trk = 0;

		Track = trk;
	}

	// Move to next line
	Line++;
	if (Line >= kTrackLines || LineJump >= 0) {

		if (LineJump >= 0)
			Line = LineJump;
		else
			Line = 0;

		// Move to next track in order list
		Order++;
		Track = GetTrack();
		if (Line > 0)
			Track = SkipToLine(Track, Line, false);
	}
}



//==================================================================================================
// Play a single note.  Returns the line number in the next pattern to jump to if a jump command was
// found, or -1 if none.
//==================================================================================================
void RADPlayer::PlayNote(int channum, int8_t notenum, int8_t octave, uint16_t instnum, uint8_t cmd, uint8_t param, e_Source src, int op) {
	CChannel &chan = Channels[channum];

	// Recursion detector.  This is needed as riffs can trigger other riffs, and they could end up
	// in a loop
	if (Entrances >= 8)
		return;
	Entrances++;

	// Select which effects source we're using
	CEffects *fx = &chan.FX;
	if (src == SRiff)
		fx = &chan.Riff.FX;
	else if (src == SIRiff)
		fx = &chan.IRiff.FX;

	bool transposing = false;

	// For tone-slides the note is the target
	if (cmd == cmToneSlide) {
		if (notenum > 0 && notenum <= 12) {
			fx->ToneSlideOct = octave;
			fx->ToneSlideFreq = NoteFreq[notenum - 1];
		}
		goto toneslide;
	}

	// Playing a new instrument?
	if (instnum > 0) {
		CInstrument *oldinst = chan.Instrument;
		CInstrument *inst = &Instruments[instnum - 1];
		chan.Instrument = inst;

		if (inst->Algorithm < 7) {

			LoadInstrumentOPL3(channum);

			// Bounce the channel
			chan.KeyFlags |= fKeyOff | fKeyOn;

			ResetFX(&chan.IRiff.FX);

			if (src != SIRiff || inst != oldinst) {

				// Instrument riff?
				if (inst->Riff && inst->RiffSpeed > 0) {

					chan.IRiff.Track = chan.IRiff.TrackStart = inst->Riff;
					chan.IRiff.Line = 0;
					chan.IRiff.Speed = inst->RiffSpeed;
					chan.IRiff.LastInstrument = 0;

					// Note given with riff command is used to transpose the riff
					if (notenum >= 1 && notenum <= 12) {
						chan.IRiff.TransposeOctave = octave;
						chan.IRiff.TransposeNote = notenum;
						transposing = true;
					}
					else {
						chan.IRiff.TransposeOctave = 3;
						chan.IRiff.TransposeNote = 12;
					}

					// Do first tick of riff
					chan.IRiff.SpeedCnt = 1;
					TickRiff(channum, chan.IRiff, false);

				}
				else
					chan.IRiff.SpeedCnt = 0;
			}
		}
		else
			// Ignore MIDI instruments
			chan.Instrument = 0;
	}

	// Starting a channel riff?
	if (cmd == cmRiff || cmd == cmTranspose) {

		ResetFX(&chan.Riff.FX);

		uint8_t p0 = param / 10;
		uint8_t p1 = param % 10;
		chan.Riff.Track = p1 > 0 ? Riffs[p0][p1 - 1] : 0;
		if (chan.Riff.Track) {

			chan.Riff.TrackStart = chan.Riff.Track;
			chan.Riff.Line = 0;
			chan.Riff.Speed = Speed;
			chan.Riff.LastInstrument = 0;

			// Note given with riff command is used to transpose the riff
			if (cmd == cmTranspose && notenum >= 1 && notenum <= 12) {
				chan.Riff.TransposeOctave = octave;
				chan.Riff.TransposeNote = notenum;
				transposing = true;
			}
			else {
				chan.Riff.TransposeOctave = 3;
				chan.Riff.TransposeNote = 12;
			}

			// Do first tick of riff
			chan.Riff.SpeedCnt = 1;
			TickRiff(channum, chan.Riff, true);

		}
		else
			chan.Riff.SpeedCnt = 0;
	}

	// Play the note
	if (!transposing && notenum > 0) {

		// Key-off?
		if (notenum == 15)
			chan.KeyFlags |= fKeyOff;

		if (!chan.Instrument || chan.Instrument->Algorithm < 7)
			PlayNoteOPL3(channum, octave, notenum);
	}

	// Process effect
	switch (cmd) {

	case cmSetVol:
		SetVolume(channum, param);
		break;

	case cmSetSpeed:
		if (src == SNone) {
			Speed = param;
			SpeedCnt = param;
		}
		else if (src == SRiff) {
			chan.Riff.Speed = param;
			chan.Riff.SpeedCnt = param;
		}
		else if (src == SIRiff) {
			chan.IRiff.Speed = param;
			chan.IRiff.SpeedCnt = param;
		}
		break;

	case cmPortamentoUp:
		fx->PortSlide = param;
		break;

	case cmPortamentoDwn:
		fx->PortSlide = -int8_t(param);
		break;

	case cmToneVolSlide:
	case cmVolSlide: {
		int8_t val = param;
		if (val >= 50)
			val = -(val - 50);
		fx->VolSlide = val;
		if (cmd != cmToneVolSlide)
			break;
	}
					 // Fall through!

	case cmToneSlide: {
	toneslide:
		uint8_t speed = param;
		if (speed)
			fx->ToneSlideSpeed = speed;
		GetSlideDir(channum, fx);
		break;
	}

	case cmJumpToLine: {
		if (param >= kTrackLines)
			break;

		// Note: jump commands in riffs are checked for within TickRiff()
		if (src == SNone)
			LineJump = param;

		break;
	}

	case cmMultiplier: {
		if (src == SIRiff)
			LoadInstMultiplierOPL3(channum, op, param);
		break;
	}

	case cmVolume: {
		if (src == SIRiff)
			LoadInstVolumeOPL3(channum, op, param);
		break;
	}

	case cmFeedback: {
		if (src == SIRiff) {
			uint8_t which = param / 10;
			uint8_t fb = param % 10;
			LoadInstFeedbackOPL3(channum, which, fb);
		}
		break;
	}
	}

	Entrances--;
}



//==================================================================================================
// Sets the OPL3 registers for a given instrument.
//==================================================================================================
void RADPlayer::LoadInstrumentOPL3(int channum) {
	CChannel &chan = Channels[channum];

	const CInstrument *inst = chan.Instrument;
	if (!inst)
		return;

	uint8_t alg = inst->Algorithm;
	chan.Volume = inst->Volume;
	chan.DetuneA = (inst->Detune + 1) >> 1;
	chan.DetuneB = inst->Detune >> 1;

	// Turn on 4-op mode for algorithms 2 and 3 (algorithms 4 to 6 are simulated with 2-op mode)
	if (UseOPL3 && channum < 6) {
		uint8_t mask = 1 << channum;
		SetOPL3(0x104, (GetOPL3(0x104) & ~mask) | (alg == 2 || alg == 3 ? mask : 0));
	}

	// Left/right/feedback/algorithm
	if (UseOPL3) {
		SetOPL3(0xC0 + ChanOffsets3[channum], ((inst->Panning[1] ^ 3) << 4) | inst->Feedback[1] << 1 | (alg == 3 || alg == 5 || alg == 6 ? 1 : 0));
		SetOPL3(0xC0 + Chn2Offsets3[channum], ((inst->Panning[0] ^ 3) << 4) | inst->Feedback[0] << 1 | (alg == 1 || alg == 6 ? 1 : 0));
	}
	else {
		SetOPL3(0xC0 + channum, ((inst->Panning[0] ^ 3) << 4) | inst->Feedback[0] << 1 | (alg == 1 ? 1 : 0));
	}

	// Load the operators
	for (int i = 0; i < (UseOPL3 ? 4 : 2); i++) {

		static const uint8_t blank[] = { 0, 0x3F, 0, 0xF0, 0 };
		const uint8_t *op = (alg < 2 && i >= 2) ? blank : inst->Operators[i];
		uint16_t reg = UseOPL3 ? OpOffsets3[channum][i] : OpOffsets2[channum][i];

		uint16_t vol = ~op[1] & 0x3F;

		// Do volume scaling for carriers
		if (AlgCarriers[alg][i]) {
			vol = vol * inst->Volume / 64;
			vol = vol * MasterVol / 64;
		}

		SetOPL3(reg + 0x20, op[0]);
		SetOPL3(reg + 0x40, (op[1] & 0xC0) | ((vol ^ 0x3F) & 0x3F));
		SetOPL3(reg + 0x60, op[2]);
		SetOPL3(reg + 0x80, op[3]);
		SetOPL3(reg + 0xE0, op[4]);
	}
}



//==================================================================================================
// Play note on OPL3 hardware.
//==================================================================================================
void RADPlayer::PlayNoteOPL3(int channum, int8_t octave, int8_t note) {
	CChannel &chan = Channels[channum];

	uint16_t o1, o2;
	if (UseOPL3) {
		o1 = ChanOffsets3[channum];
		o2 = Chn2Offsets3[channum];
	}
	else {
		o1 = 0;
		o2 = channum;
	}

	// Key off the channel
	if (chan.KeyFlags & fKeyOff) {
		chan.KeyFlags &= ~(fKeyOff | fKeyedOn);
		if (UseOPL3)
			SetOPL3(0xB0 + o1, GetOPL3(0xB0 + o1) & ~0x20);
		SetOPL3(0xB0 + o2, GetOPL3(0xB0 + o2) & ~0x20);
	}

	if (note > 12)
		return;

	bool op4 = (UseOPL3 && chan.Instrument && chan.Instrument->Algorithm >= 2);

	uint16_t freq = NoteFreq[note - 1];
	uint16_t frq2 = freq;

	chan.CurrFreq = freq;
	chan.CurrOctave = octave;

	// Detune.  We detune both channels in the opposite direction so the note retains its tuning
	freq += chan.DetuneA;
	frq2 -= chan.DetuneB;

	// Frequency low byte
	if (op4)
		SetOPL3(0xA0 + o1, frq2 & 0xFF);
	SetOPL3(0xA0 + o2, freq & 0xFF);

	// Frequency high bits + octave + key on
	if (chan.KeyFlags & fKeyOn)
		chan.KeyFlags = (chan.KeyFlags & ~fKeyOn) | fKeyedOn;
	if (op4)
		SetOPL3(0xB0 + o1, (frq2 >> 8) | (octave << 2) | ((chan.KeyFlags & fKeyedOn) ? 0x20 : 0));
	else if (UseOPL3)
		SetOPL3(0xB0 + o1, 0);
	SetOPL3(0xB0 + o2, (freq >> 8) | (octave << 2) | ((chan.KeyFlags & fKeyedOn) ? 0x20 : 0));
}



//==================================================================================================
// Prepare FX for new line.
//==================================================================================================
void RADPlayer::ResetFX(CEffects *fx) {
	fx->PortSlide = 0;
	fx->VolSlide = 0;
	fx->ToneSlideDir = 0;
}



//==================================================================================================
// Tick the channel riff.
//==================================================================================================
void RADPlayer::TickRiff(int channum, CChannel::CRiff &riff, bool chan_riff) {
	uint8_t lineid;

	if (riff.SpeedCnt == 0) {
		ResetFX(&riff.FX);
		return;
	}

	riff.SpeedCnt--;
	if (riff.SpeedCnt > 0)
		return;
	riff.SpeedCnt = riff.Speed;

	uint8_t line = riff.Line++;
	if (riff.Line >= kTrackLines)
		riff.SpeedCnt = 0;

	ResetFX(&riff.FX);

	// Is this the current line in track?
	uint8_t *trk = riff.Track;
	if (trk && (*trk & 0x7F) == line) {
		lineid = *trk++;

		if (chan_riff) {

			// Channel riff: play current note
			UnpackNote(trk, riff.LastInstrument);
			Transpose(riff.TransposeNote, riff.TransposeOctave);
			PlayNote(channum, NoteNum, OctaveNum, InstNum, EffectNum, Param, SRiff);

		}
		else {

			// Instrument riff: here each track channel is an extra effect that can run, but is not
			// actually a different physical channel
			bool last;
			do {
				int col = *trk & 15;
				last = UnpackNote(trk, riff.LastInstrument);
				if (EffectNum != cmIgnore)
					Transpose(riff.TransposeNote, riff.TransposeOctave);
				PlayNote(channum, NoteNum, OctaveNum, InstNum, EffectNum, Param, SIRiff, col > 0 ? (col - 1) & 3 : 0);
			} while (!last);
		}

		// Last line?
		if (lineid & 0x80)
			trk = 0;

		riff.Track = trk;
	}

	// Special case; if next line has a jump command, run it now
	if (!trk || (*trk++ & 0x7F) != riff.Line)
		return;

	UnpackNote(trk, lineid); // lineid is just a dummy here
	if (EffectNum == cmJumpToLine && Param < kTrackLines) {
		riff.Line = Param;
		riff.Track = SkipToLine(riff.TrackStart, Param, chan_riff);
	}
}



//==================================================================================================
// This continues any effects that operate continuously (eg. slides).
//==================================================================================================
void RADPlayer::ContinueFX(int channum, CEffects *fx) {
	CChannel &chan = Channels[channum];

	if (fx->PortSlide)
		Portamento(channum, fx, fx->PortSlide, false);

	if (fx->VolSlide) {
		int8_t vol = chan.Volume;
		vol -= fx->VolSlide;
		if (vol < 0)
			vol = 0;
		SetVolume(channum, vol);
	}

	if (fx->ToneSlideDir)
		Portamento(channum, fx, fx->ToneSlideDir, true);
}



//==================================================================================================
// Sets the volume of given channel.
//==================================================================================================
void RADPlayer::SetVolume(int channum, uint8_t vol) {
	CChannel &chan = Channels[channum];

	// Ensure volume is within range
	if (vol > 64)
		vol = 64;

	chan.Volume = vol;

	// Scale volume to master volume
	vol = vol * MasterVol / 64;

	CInstrument *inst = chan.Instrument;
	if (!inst)
		return;
	uint8_t alg = inst->Algorithm;

	// Set volume of all carriers
	for (int i = 0; i < 4; i++) {
		uint8_t *op = inst->Operators[i];

		// Is this operator a carrier?
		if (!AlgCarriers[alg][i])
			continue;

		uint8_t opvol = uint16_t((op[1] & 63) ^ 63) * vol / 64;
		uint16_t reg = 0x40 + (UseOPL3 ? OpOffsets3[channum][i] : OpOffsets2[channum][i]);
		SetOPL3(reg, (GetOPL3(reg) & 0xC0) | (opvol ^ 0x3F));
	}
}



//==================================================================================================
// Starts a tone-slide.
//==================================================================================================
void RADPlayer::GetSlideDir(int channum, CEffects *fx) {
	CChannel &chan = Channels[channum];

	int8_t speed = fx->ToneSlideSpeed;
	if (speed > 0) {
		uint8_t oct = fx->ToneSlideOct;
		uint16_t freq = fx->ToneSlideFreq;

		uint16_t oldfreq = chan.CurrFreq;
		uint8_t oldoct = chan.CurrOctave;

		if (oldoct > oct)
			speed = -speed;
		else if (oldoct == oct) {
			if (oldfreq > freq)
				speed = -speed;
			else if (oldfreq == freq)
				speed = 0;
		}
	}

	fx->ToneSlideDir = speed;
}



//==================================================================================================
// Load multiplier value into operator.
//==================================================================================================
void RADPlayer::LoadInstMultiplierOPL3(int channum, int op, uint8_t mult) {
	// Always assume OPL3 register numbering since this is not used in v1 tunes
	uint16_t reg = 0x20 + OpOffsets3[channum][op];
	SetOPL3(reg, (GetOPL3(reg) & 0xF0) | (mult & 15));
}



//==================================================================================================
// Load volume value into operator.
//==================================================================================================
void RADPlayer::LoadInstVolumeOPL3(int channum, int op, uint8_t vol) {
	// Always assume OPL3 register numbering since this is not used in v1 tunes
	uint16_t reg = 0x40 + OpOffsets3[channum][op];
	SetOPL3(reg, (GetOPL3(reg) & 0xC0) | ((vol & 0x3F) ^ 0x3F));
}



//==================================================================================================
// Load feedback value into instrument.
//==================================================================================================
void RADPlayer::LoadInstFeedbackOPL3(int channum, int which, uint8_t fb) {
	// Always assume OPL3 register numbering since this is not used in v1 tunes
	if (which == 0) {

		uint16_t reg = 0xC0 + Chn2Offsets3[channum];
		SetOPL3(reg, (GetOPL3(reg) & 0x31) | ((fb & 7) << 1));

	}
	else if (which == 1) {

		uint16_t reg = 0xC0 + ChanOffsets3[channum];
		SetOPL3(reg, (GetOPL3(reg) & 0x31) | ((fb & 7) << 1));
	}
}



//==================================================================================================
// This adjusts the pitch of the given channel's note.  There may also be a limiting value on the
// portamento (for tone slides).
//==================================================================================================
void RADPlayer::Portamento(uint16_t channum, CEffects *fx, int8_t amount, bool toneslide) {
	CChannel &chan = Channels[channum];

	uint16_t freq = chan.CurrFreq;
	uint8_t oct = chan.CurrOctave;

	freq += amount;

	if (freq < 0x156) {

		if (oct > 0) {
			oct--;
			freq += 0x2AE - 0x156;
		}
		else
			freq = 0x156;

	}
	else if (freq > 0x2AE) {

		if (oct < 7) {
			oct++;
			freq -= 0x2AE - 0x156;
		}
		else
			freq = 0x2AE;
	}

	if (toneslide) {

		if (amount >= 0) {

			if (oct > fx->ToneSlideOct || (oct == fx->ToneSlideOct && freq >= fx->ToneSlideFreq)) {
				freq = fx->ToneSlideFreq;
				oct = fx->ToneSlideOct;
			}

		}
		else {

			if (oct < fx->ToneSlideOct || (oct == fx->ToneSlideOct && freq <= fx->ToneSlideFreq)) {
				freq = fx->ToneSlideFreq;
				oct = fx->ToneSlideOct;
			}
		}
	}

	chan.CurrFreq = freq;
	chan.CurrOctave = oct;

	// Apply detunes
	uint16_t frq2 = freq - chan.DetuneB;
	freq += chan.DetuneA;

	// Write value back to OPL3
	uint16_t chan_offset = UseOPL3 ? Chn2Offsets3[channum] : channum;
	SetOPL3(0xA0 + chan_offset, freq & 0xFF);
	SetOPL3(0xB0 + chan_offset, (freq >> 8 & 3) | oct << 2 | (GetOPL3(0xB0 + chan_offset) & 0xE0));

	if (UseOPL3) {
		chan_offset = ChanOffsets3[channum];
		SetOPL3(0xA0 + chan_offset, frq2 & 0xFF);
		SetOPL3(0xB0 + chan_offset, (frq2 >> 8 & 3) | oct << 2 | (GetOPL3(0xB0 + chan_offset) & 0xE0));
	}
}



//==================================================================================================
// Transpose the note returned by UnpackNote().
// Note: due to RAD's wonky legacy middle C is octave 3 note number 12.
//==================================================================================================
void RADPlayer::Transpose(int8_t note, int8_t octave) {

	if (NoteNum >= 1 && NoteNum <= 12) {

		int8_t toct = octave - 3;
		if (toct != 0) {
			OctaveNum += toct;
			if (OctaveNum < 0)
				OctaveNum = 0;
			else if (OctaveNum > 7)
				OctaveNum = 7;
		}

		int8_t tnot = note - 12;
		if (tnot != 0) {
			NoteNum += tnot;
			if (NoteNum < 1) {
				NoteNum += 12;
				if (OctaveNum > 0)
					OctaveNum--;
				else
					NoteNum = 1;
			}
		}
	}
}



//==================================================================================================
// Compute total time of tune if it didn't repeat.  Note, this stops the tune so should only be done
// prior to initial playback.
//==================================================================================================
#if RAD_DETECT_REPEATS
static void RADPlayerDummyOPL3(void *arg, uint16_t reg, uint8_t data) {}
//--------------------------------------------------------------------------------------------------
uint32_t RADPlayer::ComputeTotalTime() {

	Stop();
	void(*old_opl3)(void *, uint16_t, uint8_t) = OPL3;
	OPL3 = RADPlayerDummyOPL3;

	while (!Update())
		;
	uint32_t total = PlayTime;

	Stop();
	OPL3 = old_opl3;

	return total / Hertz;
}
#endif

/*
*
* Start of RADPlayer wrapper for AdPlug
*
*/

static void writeOPL(void *arg, uint16_t reg, uint8_t val) {
	Copl *opl = (Copl *)arg;
	int chip = reg >> 8;
	if (opl->getchip() != chip)
		opl->setchip(chip);
	opl->write(reg & 0xFF, val);
}

CPlayer *Crad2Player::factory(Copl *newopl) {
	return new Crad2Player(newopl);
}

Crad2Player::Crad2Player(Copl *newopl)
	: CPlayer(newopl) {

	rad = new RADPlayer();
	data = 0;
}

Crad2Player::~Crad2Player() {
	delete rad;
	delete[] data;
}

bool Crad2Player::load(const std::string &filename, const CFileProvider &fp) {
	char *newdata;
	size_t size;
	const char *err;

	binistream *file = fp.open(filename);
	if (!file) return false;

	char header[17];
	header[16] = '\0';
	file->readString(header, 16);
	if (strncmp(header, "RAD by REALiTY!!", 16)) {
		fp.close(file);
		return false;
	}

	file->seek(0, binio::End);
	size = file->pos();

	// some old tunes have a truncated final note for some reason
	// add an extra empty byte at the end of the file so they still "work"
	newdata = new char[size + 1];
	newdata[size] = '\0';
	file->seek(0);
	file->readString(newdata, size);
	fp.close(file);

	if (!(err = RADValidate(newdata, size + 1))) {
		rad->Init(newdata, writeOPL, opl);
		// playback timer is < 0 if load failed
		if (rad->GetHertz() > 0) {
			delete[] data;
			data = newdata;

			// read description
			desc.clear();
			const unsigned char *newdesc = rad->GetDescription();
			while (newdesc && *newdesc) {
				unsigned char c = *newdesc++;

				if (c >= 0x20)
					desc += c;
				else if (c == 0x01)
					desc += '\n';
				else for (int i = 0; i < c; i++)
					desc += ' ';
			}

			// done
			return true;
		}
	}
	else {
		AdPlug_LogWrite("Crad2Player::load(\"%s\"): %s\n", filename.c_str(), err);
	}

	delete[] newdata;
	return false;
}

bool Crad2Player::update() { return !rad->Update(); }
void Crad2Player::rewind(int) { rad->Stop(); }
float Crad2Player::getrefresh() { return rad->GetHertz(); }

std::string Crad2Player::gettype() {
	char type[64];
	snprintf(type, sizeof(type), "Reality ADlib Tracker (version %d)", rad->GetVersion());
	return std::string(type);
}
unsigned int Crad2Player::getpatterns() { return rad->GetTunePatterns(); }
unsigned int Crad2Player::getpattern() { return rad->GetTunePattern(); }
unsigned int Crad2Player::getorders() { return rad->GetTuneLength(); }
unsigned int Crad2Player::getorder() { return rad->GetTunePos(); }
unsigned int Crad2Player::getrow() { return rad->GetTuneLine(); }
unsigned int Crad2Player::getspeed() { return rad->GetSpeed(); }
unsigned int Crad2Player::getinstruments() { return rad->GetTuneInsts(); }
std::string Crad2Player::getinstrument(unsigned int n) { return rad->GetTuneInst(n); }
