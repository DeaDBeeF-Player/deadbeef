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
 * coktel.h - Coktel Vision ADL player by Stas'M <binarymaster@mail.ru>
 *
 * REFERENCES:
 * http://www.vgmpf.com/Wiki/index.php?title=ADL_(Coktel_Vision)
 * https://github.com/DrMcCoy/CoktelADL2VGM/blob/master/src/adlib/adlplayer.cpp
 * https://github.com/scummvm/scummvm/blob/master/engines/gob/sound/adlplayer.cpp
 */

#ifndef H_ADPLUG_COKTELPLAYER
#define H_ADPLUG_COKTELPLAYER

#include "composer.h"

#define COK_NOTE_ON_VOL		0x00
#define COK_NOTE_OFF		0x80
#define COK_NOTE_ON			0x90
#define COK_PITCH_BEND		0xA0
#define COK_VOLUME_SLIDE	0xB0
#define COK_TIMBRE_CHANGE	0xC0
#define COK_MODIFY_TIMBRE	0xD0
#define COK_SET_MOD_TIMBRE	0xFE
#define COK_END_OF_SONG		0xFF

#define TIMBRE_DEF_SIZE 	(ADLIB_INST_LEN * sizeof(int16_t))	// 28 * 2 = 56
#define COK_HEADER_LEN		3
#define COK_MIN_SIZE 		(COK_HEADER_LEN + TIMBRE_DEF_SIZE + 1)	// 60

class CcoktelPlayer: public CcomposerBackend
{
public:
	static CPlayer *factory(Copl *newopl);

	CcoktelPlayer(Copl *newopl)
		: CcomposerBackend(newopl), data(0), insts(0), modifyTimbre(0xff)
		{ }
	~CcoktelPlayer()
	{
		if (insts) delete [] insts;
		if (data) delete [] data;
	};

	bool load(const std::string &filename, const CFileProvider &fp);
	bool update();
	void frontend_rewind(int subsong);

	float getrefresh()
	{
		return 1000.0f;
	};

	std::string gettype()
	{
		return std::string("AdLib Visual Composer: Coktel Vision");
	}

	unsigned int getinstruments()
	{
		return insts ? nrTimbre : 0;
	};

private:
	void executeCommand();

protected:
	unsigned long	pos, size;
	bool		songend;
	bool		first_tick;
	uint8_t *	data;					/* MIDI data */
	uint8_t		soundMode;				/* 0: melodic, 1: percussive */
	uint8_t		nrTimbre;				/* # of timbres */

	uint32_t	counter;				/* tick counter */
	uint32_t	ticks;					/* ticks to wait for next event */
	uint8_t		timbre[MAX_VOICES];		/* actual instrument of all voices */

	/* structure for timbres */
	struct adl_inst {
		uint8_t	initial[ADLIB_INST_LEN];
		uint8_t	modified[ADLIB_INST_LEN];
		int		backend_index;
	};

	adl_inst *	insts;					/* instrument definitions */
	uint8_t		modifyTimbre;			/* this instrument index will be modified */
};

#endif
