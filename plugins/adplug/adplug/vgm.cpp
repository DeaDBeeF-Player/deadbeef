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
 * vgm.c - VGM Player by Stas'M <binarymaster@mail.ru>
 *
 * REFERENCES:
 * https://vgmrips.net/wiki/VGM_Specification
 * http://www.smspower.org/uploads/Music/vgmspec160.txt
 * http://www.smspower.org/uploads/Music/gd3spec100.txt
 *
 * TODO:
 * - Add support for *.VGZ (GZip compressed files)
 */

#include <cstring>
#include <stdio.h>
#include <stdlib.h>

#include "vgm.h"

/*** public methods *************************************/

CPlayer *CvgmPlayer::factory(Copl *newopl)
{
	return new CvgmPlayer(newopl);
}

void fillGD3Tag(binistream *f, wchar_t *data)
{
	uint16_t chr, cnt = 0;
	do
	{
		chr = f->readInt(2);
		data[cnt < 256 ? cnt : 255] = cnt < 256 ? chr : 0;
		cnt++;
	} while (chr && !f->eof());
}

bool CvgmPlayer::load(const std::string &filename, const CFileProvider &fp)
{
	binistream *f = fp.open(filename);
	if (!f) return false;

	if (!fp.extension(filename, ".vgm") &&
		!fp.extension(filename, ".vgz"))
	{
		fp.close(f);
		return false;
	}

	if (fp.filesize(f) < VGM_GZIP_MIN)
	{
		// File size is too small
		fp.close(f);
		return false;
	}
	char id[4];
	f->readString(id, 4);

	if ((uint8_t)id[0] == 0x1F &&
		(uint8_t)id[1] == 0x8B)
	{
		// TODO: GZip decompression
		fp.close(f);
		return false;
	}
	if (fp.filesize(f) < VGM_HEADER_MIN)
	{
		// File size is too small
		fp.close(f);
		return false;
	}
	if (strncmp(id, VGM_HEADER_ID, 4))
	{
		// Header ID mismatch
		fp.close(f);
		return false;
	}
	uint32_t eof = f->readInt(4);
	if (fp.filesize(f) != eof + OFFSET_EOF)
	{
		// EOF offset is incorrect
		fp.close(f);
		return false;
	}
	version = f->readInt(4);
	if (version < 0x151)
	{
		// Minimum supported VGM version is 1.51
		fp.close(f);
		return false;
	}
	f->seek(12, binio::Add);
	samples = f->readInt(4);
	loop_ofs = f->readInt(4);
	loop_smp = f->readInt(4);
	rate = f->readInt(4);

	f->seek(12, binio::Add);
	int data_ofs = f->readInt(4);
	if (data_ofs < OFFSET_YM3812 - OFFSET_DATA + 4)
	{
		// VGM data overlays important header fields
		fp.close(f);
		return false;
	}
	clock = 0;
	if (data_ofs >= OFFSET_YMF262 - OFFSET_DATA + 4)
	{
		f->seek(OFFSET_YMF262);
		clock = f->readInt(4);
	}
	vgmOPL3 = clock != 0;
	vgmDual = false;
	if (!vgmOPL3)
	{
		f->seek(OFFSET_YM3812);
		clock = f->readInt(4);
		vgmDual = (clock & VGM_DUAL_BIT) > 0;
	}
	clock &= (VGM_DUAL_BIT - 1);
	if (!clock)
	{
		// VGM clock is not set
		fp.close(f);
		return false;
	}
	loop_base = 0;
	if (data_ofs >= OFFSET_LOOPBASE - OFFSET_DATA + 1)
	{
		f->seek(OFFSET_LOOPBASE);
		loop_base = f->readInt(1);
	}
	loop_mod = 0;
	if (data_ofs >= OFFSET_LOOPMOD - OFFSET_DATA + 1)
	{
		f->seek(OFFSET_LOOPMOD);
		loop_mod = f->readInt(1);
	}
	data_sz = 0;
	f->seek(OFFSET_GD3);
	int gd3_ofs = f->readInt(4);
	if (gd3_ofs)
	{
		// process GD3
		f->seek(OFFSET_GD3 + gd3_ofs);
		f->readString(id, 4);
		if (!strncmp(id, GD3_HEADER_ID, 4))
		{
			/* int gd3_ver = */ f->readInt(4);
			/* int gd3_size = */ f->readInt(4);
			fillGD3Tag(f, GD3.title_en);
			fillGD3Tag(f, GD3.title_jp);
			fillGD3Tag(f, GD3.game_en);
			fillGD3Tag(f, GD3.game_jp);
			fillGD3Tag(f, GD3.system_en);
			fillGD3Tag(f, GD3.system_jp);
			fillGD3Tag(f, GD3.author_en);
			fillGD3Tag(f, GD3.author_jp);
			fillGD3Tag(f, GD3.date);
			fillGD3Tag(f, GD3.ripper);
			fillGD3Tag(f, GD3.notes);
		}
	}
	else
	{
		f->seek(OFFSET_EOF);
		gd3_ofs = f->readInt(4);
	}
	f->seek(OFFSET_DATA + data_ofs);
	data_sz = gd3_ofs - data_ofs;
	vgmData = new uint8_t[data_sz];
	for (int i = 0; i < data_sz; i++)
	{
		vgmData[i] = f->readInt(1);
	}
	fp.close(f);
	loop_ofs -= data_ofs + (OFFSET_DATA - OFFSET_LOOP);
	rewind(0);
	return true;
}

bool CvgmPlayer::update()
{
	uint8_t reg, val;
	wait = 0;

	do
	{
		if (pos >= data_sz)
		{
			songend = true;
			break;
		}
		uint8_t cmd = vgmData[pos++];
		switch (cmd)
		{
		case CMD_OPL2:
		case CMD_OPL3_PORT0:
			reg = vgmData[pos++];
			val = vgmData[pos++];
			if ((!vgmOPL3 && cmd == CMD_OPL2) || (vgmOPL3 && cmd == CMD_OPL3_PORT0))
			{
				if (opl->getchip() != 0)
					opl->setchip(0);
				opl->write(reg, val);
			}
			break;
		case CMD_OPL2_2ND:
		case CMD_OPL3_PORT1:
			reg = vgmData[pos++];
			val = vgmData[pos++];
			if ((vgmDual && cmd == CMD_OPL2_2ND) || (vgmOPL3 && cmd == CMD_OPL3_PORT1))
			{
				if (opl->getchip() != 1)
					opl->setchip(1);
				opl->write(reg, val);
			}
			break;
		case CMD_WAIT:
			wait  = vgmData[pos++];
			wait |= vgmData[pos++] << 8;
			break;
		case CMD_WAIT_735:
			wait = 735;
			break;
		case CMD_WAIT_882:
			wait = 882;
			break;
		case CMD_DATA_END:
			pos = data_sz;
			break;
		default:
			if (cmd >= CMD_WAIT_N && cmd <= CMD_WAIT_N + 0xF)
			{
				wait = (cmd & 0xF) + 1;
			}
		}
		if (wait && wait < 40)
			wait = 0; // skip too short pauses
		if (!songend)
			songend = pos >= data_sz;
		if (pos >= data_sz && loop_ofs >= 0)
			pos = loop_ofs;
	} while (!wait);
	return !songend;
}

void CvgmPlayer::rewind(int subsong)
{
	pos = 0; songend = false; wait = 0;
	opl->init();
}

float CvgmPlayer::getrefresh()
{
	return VGM_FREQUENCY / (wait > 0 ? wait : VGM_FREQUENCY / 1000);
}

std::string CvgmPlayer::gettype()
{
	char chip[10];
	memset(chip, 0, 10);
	if (vgmOPL3)
		strcpy(chip, "OPL3");
	else if (vgmDual)
		strcpy(chip, "Dual OPL2");
	else
		strcpy(chip, "OPL2");
	char tmpstr[40];
	uint8_t major = (version >> 8) & 0xFF;
	uint8_t minor = version & 0xFF;
	snprintf(tmpstr, sizeof(tmpstr), "Video Game Music %x.%x (%s)", major, minor, chip);
	return std::string(tmpstr);
}

std::string CvgmPlayer::gettitle()
{
	char str[256];
	str[0] = 0;
	if (GD3.title_en[0])
	{
		wcstombs(str, GD3.title_en, 256);
	}
	else if (GD3.title_jp[0])
	{
		wcstombs(str, GD3.title_jp, 256);
	}
	return std::string(str);
}

std::string CvgmPlayer::getauthor()
{
	char str[256];
	str[0] = 0;
	if (GD3.author_en[0])
	{
		wcstombs(str, GD3.author_en, 256);
	}
	else if (GD3.author_jp[0])
	{
		wcstombs(str, GD3.author_jp, 256);
	}
	return std::string(str);
}

std::string CvgmPlayer::getdesc()
{
	char game[256]; game[0] = 0;
	char system[256]; system[0] = 0;
	char date[256]; date[0] = 0;
	char notes[256]; notes[0] = 0;
	
	if (GD3.game_en[0])
	{
		wcstombs(game, GD3.game_en, 256);
	}
	else if (GD3.game_jp[0])
	{
		wcstombs(game, GD3.game_jp, 256);
	}
	if (GD3.system_en[0])
	{
		wcstombs(system, GD3.system_en, 256);
	}
	else if (GD3.system_jp[0])
	{
		wcstombs(system, GD3.system_jp, 256);
	}
	if (GD3.date[0])
		wcstombs(date, GD3.date, 256);
	if (GD3.notes[0])
		wcstombs(notes, GD3.notes, 256);
	char str_sys[256]; str_sys[0] = 0;
	if (system[0] && date[0] && strlen(system) <= 251)
	{
		snprintf(str_sys, sizeof(str_sys), "%.251s / %.*s", system, 252 - (int)strlen(system), date);
	}
	else if (system[0])
	{
		strcpy(str_sys, system);
	}
	else if (date[0])
	{
		strcpy(str_sys, date);
	}
	char str_game[256]; str_game[0] = 0;
	char str_desc[256]; str_desc[0] = 0;
	if (game[0])
	{
		if (str_sys[0] && strlen(game) <= 251)
		{
			snprintf(str_game, sizeof(str_game), "%.251s (%.*s)", game, 252 - (int)strlen(game), str_sys);
		}
		else
		{
			strcpy(str_game, game);
		}
	}
	else if (str_sys[0])
	{
		strcpy(str_game, str_sys);
	}
	if (notes[0] && strlen(str_game) <= 250)
	{
		snprintf(str_desc, sizeof(str_desc), "%.250s\r\n\r\n%.*s", str_game, 251 - (int)strlen(str_game), notes);
	}
	else
	{
		strcpy(str_desc, str_game);
	}
	return std::string(str_desc);
}
