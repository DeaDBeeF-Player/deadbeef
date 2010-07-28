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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * imf.h - IMF Player by Simon Peter <dn.tlp@gmx.net>
 */

#ifndef H_ADPLUG_IMFPLAYER
#define H_ADPLUG_IMFPLAYER

#include "player.h"

class CimfPlayer: public CPlayer
{
public:
  static CPlayer *factory(Copl *newopl);

	CimfPlayer(Copl *newopl)
	  : CPlayer(newopl), footer(0), data(0)
	  { }
	~CimfPlayer()
	  { if(data) delete [] data; if(footer) delete [] footer; };

	bool load(const char *filename, const CFileProvider &fp);
	bool update();
	void rewind(int subsong);
	float getrefresh()
	  { return timer; };

	const char * gettype()
	  { return "IMF File Format"; }
	const char * gettitle();
	const char * getauthor()
	  { return author_name; }
	const char * getdesc();

protected:
	unsigned long	pos, size;
	unsigned short	del;
	bool		songend;
	float		rate, timer;
	char		*footer;
	char track_name[100], game_name[100], author_name[100], remarks[100];
	char title[100];
	char desc[100];

	struct Sdata {
		unsigned char	reg, val;
		unsigned short	time;
	} *data;

private:
	float getrate(const char *filename, const CFileProvider &fp, binistream *f);
};

#endif
