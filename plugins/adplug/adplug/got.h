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
 * got.h - GOT Player by Stas'M <binarymaster@mail.ru>
 *
 * Based on IMF Player by Simon Peter <dn.tlp@gmx.net>
 *
 * REFERENCES:
 * http://www.shikadi.net/moddingwiki/God_of_Thunder_Music_Format
 */

#ifndef H_ADPLUG_GOTPLAYER
#define H_ADPLUG_GOTPLAYER

#include "player.h"

class CgotPlayer: public CPlayer
{
public:
	static CPlayer *factory(Copl *newopl);

	CgotPlayer(Copl *newopl)
		: CPlayer(newopl), data(0)
		{ }
	~CgotPlayer()
	{
		if(data) delete [] data;
	};

	bool load(const std::string &filename, const CFileProvider &fp);
	bool update();
	void rewind(int subsong);

	float getrefresh()
	{
		return timer;
	};

	std::string gettype()
	{
		return std::string("God of Thunder Music");
	}

	unsigned int getspeed()
	{
		return (int)rate;
	}

protected:
	unsigned long	pos, size;
	unsigned short	del;
	bool		songend;
	float		rate, timer;

	struct Sdata {
		unsigned char	time;
		unsigned char	reg, val;
	} *data;
};

#endif
