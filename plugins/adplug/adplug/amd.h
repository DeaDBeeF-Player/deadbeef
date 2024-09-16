/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2003 Simon Peter, <dn.tlp@gmx.net>, et al.
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
 * amd.h - AMD Loader by Simon Peter <dn.tlp@gmx.net>
 */

#include "strnlen.h"
#include "protrack.h"

class CamdLoader: public CmodPlayer
{
public:
  static CPlayer *factory(Copl *newopl);

	CamdLoader(Copl *newopl)
		: CmodPlayer(newopl)
	{ };

	bool load(const std::string &filename, const CFileProvider &fp);
	float getrefresh();

	std::string gettype()
	{ return std::string("AMUSIC Adlib Tracker"); }
	std::string gettitle()
	{ return std::string(songname, strnlen(songname, sizeof(songname))); }
	std::string getauthor()
	{ return std::string(author, strnlen(author, sizeof(author))); }
	unsigned int getinstruments()
	{ return 26; }
	std::string getinstrument(unsigned int n)
	{ return n < getinstruments() ? std::string(instname[n], strnlen(instname[n], sizeof(instname[n]))) : std::string(); }

private:
	char songname[24], author[24], instname[26][23];
};
