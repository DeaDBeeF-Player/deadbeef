/*
* Adplug - Replayer for many OPL2/OPL3 audio file formats.
* Copyright (C) 1999 - 2007 Simon Peter, <dn.tlp@gmx.net>, et al.
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
* rad2.h - RAD v2 replayer class
*/

#ifndef H_ADPLUG_RAD2PLAYER
#define H_ADPLUG_RAD2PLAYER

#include "player.h"

class RADPlayer;

class Crad2Player: public CPlayer
{
public:
	static CPlayer *factory(Copl *newopl);

	Crad2Player(Copl *newopl);
	virtual ~Crad2Player();

	/***** Operational methods *****/
	bool load(const std::string &filename, const CFileProvider &fp = CProvider_Filesystem());
	bool update();
	void rewind(int subsong = -1);
	float getrefresh();

	/***** Informational methods *****/
	std::string gettype();
	std::string getdesc()         { return desc; }

	unsigned int getpatterns();
	unsigned int getinstruments();
	unsigned int getpattern();
	unsigned int getorders();
	unsigned int getorder();
	unsigned int getrow();
	unsigned int getspeed();
	std::string getinstrument(unsigned int n);

protected:
	RADPlayer *rad;
	char *data;

	std::string desc;

};

#endif
