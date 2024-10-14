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
 * mdi.h - AdLib SMF (MDI) Player by Stas'M <binarymaster@mail.ru>
 *
 * Based on MIDIPLAY.C by Dale Glowinski, Ad Lib Inc.
 *
 * REFERENCES:
 * http://www.shikadi.net/moddingwiki/MDI_Format
 * http://www.vgmpf.com/Wiki/index.php?title=MDI
 */

#ifndef H_ADPLUG_MDIPLAYER
#define H_ADPLUG_MDIPLAYER

#include "composer.h"

#define MIDI_CHUNK_SIZE	4	/* FOURCC size */
#define MIDI_HEAD_SIZE	6	/* MThd data size */
#define MIDI_MIN_SIZE	MIDI_CHUNK_SIZE + sizeof(uint32_t) + MIDI_HEAD_SIZE + MIDI_CHUNK_SIZE + sizeof(uint32_t)
#define MIDI_DEF_TEMPO	500000

#define NR_CHANS		16

#define NOTE_OFF		0x80
#define NOTE_ON			0x90
#define AFTER_TOUCH		0xA0
#define CONTROL_CHANGE	0xB0
#define PROG_CHANGE		0xC0
#define CHANNEL_PRESSURE	0xD0
#define PITCH_BEND		0xE0
#define SYSEX_F0		0xf0
#define SYSEX_F7		0xf7
#define STOP_FC			0xfc
#define META			0xff

#define END_OF_TRACK	0x2f
#define TEMPO			0x51
#define SEQ_SPECIFIC	0x7f

#define META_SIGN_LEN	3
#define META_CODE_LEN	2
#define META_MIN_SIZE	META_SIGN_LEN + META_CODE_LEN + 1

#define ADLIB_TIMBRE	1
#define ADLIB_RHYTHM	2
#define ADLIB_PITCH		3

class CmdiPlayer: public CcomposerBackend
{
public:
	static CPlayer *factory(Copl *newopl);

	CmdiPlayer(Copl *newopl)
		: CcomposerBackend(newopl), data(0)
		{ }
	~CmdiPlayer()
	{
		if(data) delete [] data;
	};

	bool load(const std::string &filename, const CFileProvider &fp);
	bool update();
	void frontend_rewind(int subsong);

	float getrefresh()
	{
		return timer;
	};

	std::string gettype()
	{
		return std::string("AdLib Visual Composer: MIDIPlay File");
	}

private:
	void SetTempo(uint32_t tempo);
	uint32_t GetVarVal();
	void executeCommand();

protected:
	unsigned long	pos, size;
	bool		songend;
	float		timer;
	uint16_t	division;				/* division in PPQN */
	uint8_t *	data;					/* MIDI data */

	uint32_t	counter;					/* tick counter */
	uint32_t	ticks;					/* ticks to wait for next event */
	uint8_t		status;                 /* running status byte */
	uint8_t		volume[MAX_VOICES];		/* actual volume of all voices */
};

#endif