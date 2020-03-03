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
 * raw.h - RAW Player by Simon Peter <dn.tlp@gmx.net>
 */

/*
 * Copyright (c) 2015 - 2017 Wraithverge <liam82067@yahoo.com>
 * - Preliminary support for displaying arbitrary Tag data. (2015)
 * - Minor realignments. (2017)
 * - Corrected 'type' string. (2017)
 * - Finalized Tag support. (2017)
 */

#include "player.h"

class CrawPlayer: public CPlayer
{
public:
	static CPlayer *factory(Copl *newopl);

	CrawPlayer(Copl *newopl)
		: CPlayer(newopl), data(0)
	{ };

	~CrawPlayer()
	{ if (data) delete[] data; };

	bool load(const std::string &filename, const CFileProvider &fp);
	bool update();
	void rewind(int subsong);
	float getrefresh();

	// Wraithverge: RAC originally captured these files, not RdosPlay.
	std::string gettype() { return std::string("Raw AdLib Capture"); };

	std::string gettitle() { return std::string(title, 0, 40); };
	std::string getauthor() { return std::string(author, 0, 60); };
	std::string getdesc() { return std::string(desc, 0, 1023); };

protected:
	struct Tdata {
		unsigned char param, command;
	} *data;

	unsigned long pos, length;
	unsigned short clock, speed;
	unsigned char del;
	bool songend;

private:
	char title[40];
	char author[60];
	char desc[1023];
};
