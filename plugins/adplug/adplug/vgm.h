/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2005 Simon Peter, <dn.tlp@gmx.net>, et al.
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
 * vgm.h - VGM Player by Stas'M <binarymaster@mail.ru>
 *
 * REFERENCES:
 * https://vgmrips.net/wiki/VGM_Specification
 * http://www.smspower.org/uploads/Music/vgmspec160.txt
 * http://www.smspower.org/uploads/Music/gd3spec100.txt
 */

#ifndef H_ADPLUG_VGMPLAYER
#define H_ADPLUG_VGMPLAYER

#include "player.h"

#include <stdint.h> // for uintxx_t

#define VGM_GZIP_MIN	8		// minimum size for GZip header
#define VGM_HEADER_MIN	84		// minimum size for header (till YM3812 clock field)
#define VGM_HEADER_ID	"Vgm "	// header ident
#define OFFSET_EOF		0x04	// End Of File offset
#define OFFSET_GD3		0x14	// GD3 tag offset
#define OFFSET_LOOP		0x1C	// Loop offset
#define OFFSET_DATA		0x34	// VGM data offset
#define OFFSET_YM3812	0x50	// YM3812 clock offset
#define OFFSET_YMF262	0x5C	// YMF262 clock offset
#define OFFSET_LOOPBASE	0x7E	// Loop base offset
#define OFFSET_LOOPMOD	0x7F	// Loop modifier offset
#define VGM_DUAL_BIT	0x40000000	// VGM flag for dual-chip feature
#define GD3_HEADER_ID	"Gd3 "	// GD3 tag header ident

#define CMD_OPL2		0x5A	// YM3812, write value <dd> to register <aa>
#define CMD_OPL3_PORT0	0x5E	// YMF262 port 0, write value <dd> to register <aa>
#define CMD_OPL3_PORT1	0x5F	// YMF262 port 1, write value <dd> to register <aa>
#define CMD_WAIT		0x61	// Wait <n> samples, <n> can range from 0 to 65535 (approx 1.49 seconds)
#define CMD_WAIT_735	0x62	// wait 735 samples (60th of a second)
#define CMD_WAIT_882	0x63	// wait 882 samples (50th of a second)
#define CMD_DATA_END	0x66	// end of sound data
#define CMD_WAIT_N		0x70	// wait <n+1> samples, <n> can range from 0 to 15
#define CMD_OPL2_2ND	0xAA	// YM3812 chip 2, write value <dd> to register <aa>

#define VGM_FREQUENCY	44100.0	// VGM base sample frequency

class CvgmPlayer: public CPlayer
{
public:
	static CPlayer *factory(Copl *newopl);

	CvgmPlayer(Copl *newopl)
		: CPlayer(newopl), vgmData(0)
	{ };
	~CvgmPlayer()
	{
		if (vgmData) delete[] vgmData;
	};

	bool load(const std::string &filename, const CFileProvider &fp);
	bool update();
	void rewind(int subsong);
	float getrefresh();

	std::string gettype();
	std::string gettitle();
	std::string getauthor();
	std::string getdesc();

protected:
	int version, samples, loop_ofs, loop_smp, rate, clock;
	uint8_t loop_base, loop_mod;
	bool vgmOPL3, vgmDual;
	int data_sz;
	uint8_t * vgmData;

	struct GD3tag {
		wchar_t title_en[256], title_jp[256];
		wchar_t game_en[256], game_jp[256];
		wchar_t system_en[256], system_jp[256];
		wchar_t author_en[256], author_jp[256];
		wchar_t date[256], ripper[256], notes[256];
	};
	GD3tag GD3;

	int pos;
	bool songend;
	uint16_t wait;
};

#endif