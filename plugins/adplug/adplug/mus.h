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
 * mus.h - AdLib MIDI Music File Player by Stas'M <binarymaster@mail.ru>
 *
 * Based on PLAY.C by Marc Savary, Ad Lib Inc.
 *
 * REFERENCES:
 * http://www.shikadi.net/moddingwiki/AdLib_MIDI_Format
 * http://www.shikadi.net/moddingwiki/IMS_Format
 * http://www.shikadi.net/moddingwiki/AdLib_Timbre_Bank_Format
 * http://www.shikadi.net/moddingwiki/AdLib_Instrument_Bank_Format
 * http://www.vgmpf.com/Wiki/index.php?title=MUS_(AdLib)
 * http://www.vgmpf.com/Wiki/index.php?title=SND_(AdLib)
 */

#ifndef H_ADPLUG_MUSPLAYER
#define H_ADPLUG_MUSPLAYER

#include "composer.h"

#define SYSTEM_XOR_BYTE		0xF0
#define EOX_BYTE			0xF7
#define OVERFLOW_BYTE		0xF8
#define STOP_BYTE			0xFC

#define NOTE_OFF_BYTE		0x80
#define NOTE_ON_BYTE		0x90
#define AFTER_TOUCH_BYTE	0xA0
#define CONTROL_CHANGE_BYTE	0xB0
#define PROG_CHANGE_BYTE	0xC0
#define CHANNEL_PRESSURE_BYTE	0xD0
#define PITCH_BEND_BYTE		0xE0

#define ADLIB_CTRL_BYTE	0x7F	/* for System exclusive */
#define TEMPO_CTRL_BYTE	0

#define TUNE_NAME_SIZE		30
#define FILLER_SIZE			8
#define TIMBRE_DEF_SIZE 	(ADLIB_INST_LEN * sizeof(int16_t))
#define OVERFLOW_TICKS		240
#define MAX_SEC_DELAY		10.0f	/* Wraithverge: changed this to float, to avoid casting */
#define HEADER_LEN			70
#define SND_HEADER_LEN		6
#define IMS_SIGNATURE		0x7777

#define KNOWN_MUS_EXT		"mus", "mdy", "ims"
#define KNOWN_SND_EXT		"snd", "tim", "tbr"
#define KNOWN_SND_NAME		"", "timbres"
#define KNOWN_BNK_NAME		"", "implay", "standard"

class CmusPlayer: public CcomposerBackend
{
public:
	static CPlayer *factory(Copl *newopl);

	CmusPlayer(Copl *newopl)
		: CcomposerBackend(newopl), data(0), insts(0)
		{ }
	~CmusPlayer()
	{
		if (data) delete [] data;
		if (insts) delete[] insts;
	};

	bool load(const std::string &filename, const CFileProvider &fp);
	bool update();
	void frontend_rewind(int subsong);

	float getrefresh()
	{
		return timer;
	};

	std::string gettitle()
	{
		return std::string(tuneName);
	};

	std::string gettype();

	unsigned int getinstruments()
	{
		return insts ? nrTimbre : 0;
	};

	std::string getinstrument(unsigned int n)
	{
		return insts && n < nrTimbre ?
			(insts[n].backend_index >= 0 ?
				std::string(insts[n].name) :
				std::string(insts[n].name).append(" (missing)")
			) : std::string();
	};

private:
	bool InstsLoaded();
	bool LoadTimbreBank(const std::string fname, const CFileProvider &fp);
	bool FetchTimbreData(const std::string fname, const CFileProvider &fp);
	void SetTempo(uint16_t tempo, uint8_t tickBeat);
	uint32_t GetTicks();
	void executeCommand();

protected:
	/* variables for playback */
	unsigned long	pos;
	bool		songend;
	float		timer;

	uint32_t	counter;					/* tick counter */
	uint32_t	ticks;					/* ticks to wait for next event */
	uint8_t		status;                 /* running status byte */
	uint8_t		volume[MAX_VOICES];		/* actual volume of all voices */

	/* header variables of .MUS file */
	uint8_t		majorVersion;
	uint8_t		minorVersion;
	char		tuneName[TUNE_NAME_SIZE];
	uint8_t		tickBeat;
	uint32_t	dataSize;
	uint8_t		soundMode;			/* 0: melodic, 1: percussive */
	uint8_t		pitchBRange;		/* 1 - 12 */
	uint16_t	basicTempo;

	uint8_t *	data;				/* MIDI data */
	bool		isIMS;					/* play as IMS format */

	/* variables for timbre bank */
	struct mus_inst {
		char	name[INS_MAX_NAME_SIZE];
		int		backend_index;
	};

	uint16_t	nrTimbre;			/* # of definitions in bank. */
	mus_inst *	insts;					/* instrument definitions */
};

#endif