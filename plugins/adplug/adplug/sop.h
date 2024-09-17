/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2005 Simon Peter <dn.tlp@gmx.net>, et al.
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
 * sop.h - sopepos' Note Player by Stas'M <binarymaster@mail.ru>
 *
 * REFERENCES:
 * http://www.shikadi.net/moddingwiki/SOP_Format
 */

#ifndef H_ADPLUG_SOPPLAYER
#define H_ADPLUG_SOPPLAYER

#include <stdint.h>
#include <stdio.h>
#include "player.h"

/*
 *  Source code parts from free Ad262Sop Library (c) 1996-2005, Park Jeenhong
 */

/* percussive voice numbers: */
#define	BD      6
#define	SD      7
#define	TOM     8
#define	CYMB    9
#define	HIHAT  10

#define	MAX_VOLUME		0x7f
#define	LOG2_VOLUME		7		/* log2( MAX_VOLUME) */

#define	MID_C			60		/* MIDI standard mid C */

#define	SOP_TOM_PITCH		36		/* best frequency, in range of 0 to 95 */
#define	TOM_TO_SD		7		/* 7 half-tones between voice 7 & 8 */
#define	SOP_SD_PITCH		(SOP_TOM_PITCH + TOM_TO_SD)

#define NB_NOTES			96      /* 8 octave of 12 notes */
#define OCTAVE			12      /* half-tone by octave */
#define NB_STEP_PITCH		32      /* 32 steps between two half-tones */
#define LOG_NB_STEP_PITCH	5       /* LOG2( NB_STEP_PITCH ) */
#define NB_TABLE_DEMI_TON	OCTAVE
#define TABLE_SIZE			(NB_STEP_PITCH * NB_TABLE_DEMI_TON)

#define	maxVoices		20
#define	YMB_SIZE		80

class Cad262Driver
{
public:
	Cad262Driver(Copl *newopl)
		: opl(newopl)
	{};
	~Cad262Driver()
	{};

	void SoundWarmInit();
	void SetYM_262_SOP(int VX);
	void SetMode_SOP(int mode);
	void SetStereoPan_SOP(int chan, int value);
	void SetVoiceVolume_SOP(unsigned chan, unsigned vol);
	void SetVoiceTimbre_SOP(unsigned chan, unsigned char *array);
	void SetVoicePitch_SOP(unsigned chan, int pitch);
	void NoteOn_SOP(unsigned chan, unsigned pitch);
	void NoteOff_SOP(unsigned chan);
	int Set_4OP_Mode(unsigned chan, unsigned value);

private:
	Copl *opl;

	void UpdateFNums(int chan);
	void SndOutput1(int addr, int value);
	void SndOutput3(int addr, int value);
	void SEND_INS(int base_addr, unsigned char *value, int mode);
	void SetFreq_SOP(int voice, unsigned note, int pitch, int keyOn);

	char percussion;		/* percussion mode parameter */
	unsigned char VolumeTable[ 64 * 128 ];  /* Pre-calculated Volume Table */
	char voiceNote[20];	/* pitch of last note-on of each voice */
	char voiceKeyOn[20];	/* state of keyOn bit of each voice */
	unsigned char vPitchBend[20];	/* current pitch bend of each voice */
	unsigned char Ksl[20]; /* KSL value for Slot 1 */
	unsigned char Ksl2[20]; /* KSL value for Slot 2 */
	unsigned char Ksl2V[20]; /* Parallel connection? */
	unsigned char VoiceVolume[20], OP_MASK;
	unsigned char ymbuf[ 2 * YMB_SIZE ];
	unsigned char OP4[20];
	unsigned char Stereo[22];

protected:
	static const int fNumTbl[TABLE_SIZE];
	static const unsigned char SlotX[maxVoices * 2];
	static const unsigned char VolReg[11 * 2];
	static const unsigned MOD12[OCTAVE * 11];
	static const unsigned DIV12[OCTAVE * 8];
};

/******************************************************************/

#define SOP_HEAD_SIZE	76
#define SOP_SIGN_SIZE	7	/* "sopepos" signature */
#define SOP_FILENAME	13	/* size of file name tag */
#define SOP_TITLE		31	/* size of title tag */
#define SOP_DEF_TEMPO	120	/* default tempo BPM */
#define SOP_COMMENT	13	/* size of comment tag */
#define SOP_INSTNAME	8	/* size of instrument file name tag */
#define SOP_LONGNAME	19	/* size of instrument long name tag */
#define SOP_INST2OP	11	/* size of 2OP instrument data */
#define SOP_INST4OP	22	/* size of 4OP instrument data */
#define SOP_MAX_INST	128	/* maximum number of instruments */
#define SOP_MAX_TRACK	24	/* maximum number of tracks */
#define SOP_MAX_VOL	127	/* maximum volume */

#define SOP_CHAN_NONE	0	/* unused channel */
#define SOP_CHAN_4OP	1	/* channel in 4OP mode */
#define SOP_CHAN_2OP	2	/* channel in 2OP mode */

#define SOP_INST_4OP	0	/* 4OP instrument */
#define SOP_INST_2OP	1	/* 2OP instrument */
#define SOP_INST_BD	6	/* Bass Drum */
#define SOP_INST_SD	7	/* Snare Drum */
#define SOP_INST_TT	8	/* Tom Tom */
#define SOP_INST_CY	9	/* Cymbal */
#define SOP_INST_HH	10	/* Hi-Hat */
#define SOP_INST_WAV	11	/* Wave sample (SOP v2) */
#define SOP_INST_NONE	12	/* Unused instrument */

#define SOP_EVNT_SPEC	1	/* Special event */
#define SOP_EVNT_NOTE	2	/* Note event */
#define SOP_EVNT_TEMPO	3	/* Tempo event */
#define SOP_EVNT_VOL	4	/* Volume event */
#define SOP_EVNT_PITCH	5	/* Pitch event */
#define SOP_EVNT_INST	6	/* Instrument event */
#define SOP_EVNT_PAN	7	/* Panning event */
#define SOP_EVNT_MVOL	8	/* Master volume event */

class CsopPlayer: public CPlayer
{
public:
	static CPlayer *factory(Copl *newopl);

	CsopPlayer(Copl *newopl)
		: CPlayer(newopl), drv(0), chanMode(0), inst(0), track(0)
		{ }
	~CsopPlayer()
	{
		if (chanMode) delete[] chanMode;
		if (inst) delete[] inst;
		if (track)
		{
			for (int i = 0; i < nTracks + 1; i++)
			{
				if (track[i].data) delete[] track[i].data;
			}
			delete[] track;
		}
		if (drv) delete drv;
	};

	bool load(const std::string &filename, const CFileProvider &fp);
	bool update();
	void rewind(int subsong);

	float getrefresh()
	{
		return timer;
	};

	unsigned int getspeed()
	{
		return cur_tempo;
	};

	std::string gettitle()
	{
		return title[0] ? std::string(title) : std::string(fname);
	}

	std::string gettype()
	{
		char type[36];
		snprintf(type, sizeof(type), "Note Sequencer v%u.%u by sopepos", (version >> 8) & 0xFF, version & 0xFF);
		return std::string(type);
	}

	std::string getdesc()
	{
		return std::string(comment);
	}

	unsigned int getinstruments()
	{
		return inst ? nInsts : 0;
	};

	std::string getinstrument(unsigned int n)
	{
		return inst && n < nInsts ? std::string(inst[n].longname) : std::string();
	};

private:
	void SetTempo(uint8_t tempo);
	void executeCommand(uint8_t t);
	Cad262Driver * drv;

protected:
	bool songend;
	float timer;
	uint16_t version;
	uint8_t cur_tempo;			/* current tempo in BPM */
	uint8_t volume[SOP_MAX_TRACK];	/* actual volume of all voices */
	uint8_t lastvol[SOP_MAX_TRACK];	/* last set volume of all voices */
	uint8_t master_vol;			/* master volume */

	char fname[SOP_FILENAME];
	char title[SOP_TITLE];
	uint8_t percussive, tickBeat, basicTempo;
	char comment[SOP_COMMENT];
	uint8_t nTracks, nInsts;

	struct sop_inst {
		uint8_t type;
		char filename[SOP_INSTNAME + 1];
		char longname[SOP_LONGNAME + 1];
		uint8_t data[SOP_INST4OP];
	};
#pragma pack(push, 1)
	struct sop_sample {
		uint16_t val1;
		uint16_t val2;
		uint16_t length;
		uint16_t val4;
		uint16_t base_freq;
		uint16_t val6;
		uint16_t val7;
		uint8_t val8;
		uint16_t val9;
		uint16_t val10;
	};
#pragma pack(pop)
	struct sop_trk {
		// stored variables
		uint16_t nEvents;			/* event count */
		uint32_t size;			/* data size */
		uint8_t * data;			/* event data */

		// variables for playback
		uint32_t pos;			/* data position */
		uint32_t counter;			/* tick counter */
		uint16_t ticks;			/* ticks to wait for next event */
		uint16_t dur;			/* note play duration */
	};
	uint8_t * chanMode;			/* channel modes [nTracks] */
	sop_inst * inst;				/* instruments [nInsts] */
	sop_trk * track;				/* event tracks [nTracks + control track] */
};

#endif
