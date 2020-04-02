/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2008 Simon Peter <dn.tlp@gmx.net>, et al.
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
 * got.cpp - GOT Player by Stas'M <binarymaster@mail.ru>
 *
 * Based on IMF Player by Simon Peter <dn.tlp@gmx.net>
 *
 * REFERENCES:
 * http://www.shikadi.net/moddingwiki/God_of_Thunder_Music_Format
 */

#include <string.h>

#include "got.h"
#include "database.h"

/*** public methods *************************************/

CPlayer *CgotPlayer::factory(Copl *newopl)
{
	return new CgotPlayer(newopl);
}

bool CgotPlayer::load(const std::string &filename, const CFileProvider &fp)
{
	binistream *f = fp.open(filename); if(!f) return false;

	// file validation section
	{
		if (!fp.extension(filename, ".got"))
		{
			fp.close(f);
			return false;
		}
		if (fp.filesize(f) % 3 != 0 || fp.filesize(f) < 9)
		{
			fp.close(f);
			return false;
		}
		if (f->readInt(2) != 1)
		{
			fp.close(f);
			return false;
		}
		f->seek(fp.filesize(f) - 4);
		if (f->readInt(4) != 0)
		{
			fp.close(f);
			return false;
		}
	}
	f->seek(0);
	CAdPlugDatabase::CKey key(*f);
	f->seek(2);

	// load section
	size = fp.filesize(f) / 3 - 1;

	data = new Sdata[size];
	for(unsigned int i = 0; i < size; i++) {
		data[i].time = f->readInt(1);
		data[i].reg = f->readInt(1); data[i].val = f->readInt(1);
	}

	CAdPlugDatabase::CKey menu_music;
	menu_music.crc16 = 0xB627;
	menu_music.crc32 = 0x72036C41;

	if (key == menu_music)
		rate = 140.0f;
	else
		rate = 120.0f;

	fp.close(f);
	rewind(0);
	return true;
}

bool CgotPlayer::update()
{
	do {
		del = data[pos].time;
		opl->write(data[pos].reg, data[pos].val);
		pos++;
	} while (!del && pos < size);

	if (pos >= size) {
		pos = 0;
		songend = true;
	} else
		timer = rate / (float)del;

	return !songend;
}

void CgotPlayer::rewind(int subsong)
{
	pos = 0; del = 0; timer = rate; songend = false;
	opl->init(); opl->write(1,32); // go to OPL2 mode
}
