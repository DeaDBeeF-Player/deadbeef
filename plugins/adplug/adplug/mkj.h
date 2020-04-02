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
 * mkj.h - MKJamz Player, by Simon Peter <dn.tlp@gmx.net>
 */

#include "player.h"

class CmkjPlayer: public CPlayer
{
public:
  static CPlayer *factory(Copl *newopl);

	CmkjPlayer(Copl *newopl)
		: CPlayer(newopl), songbuf(0)
	{ }
	~CmkjPlayer()
	{ if(songbuf) delete [] songbuf; }

	bool load(const std::string &filename, const CFileProvider &fp);
	bool update();
	void rewind(int subsong);
	float getrefresh();

	std::string gettype()
	{ return std::string("MKJamz Audio File"); }

private:
	short maxchannel,maxnotes,*songbuf;
	bool songend;

	struct {
		short defined,songptr,octave,waveform,pstat,speed,delay;
	} channel[9];
};
