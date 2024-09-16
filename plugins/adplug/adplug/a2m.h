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
 * a2m.h - A2M Loader by Simon Peter <dn.tlp@gmx.net>
 */

#ifndef H_ADPLUG_A2MLOADER
#define H_ADPLUG_A2MLOADER

#include "protrack.h"
#include "sixdepack.h"

class Ca2mLoader: public CmodPlayer
{
public:
  static CPlayer *factory(Copl *newopl);

  Ca2mLoader(Copl *newopl): CmodPlayer(newopl)
    { }

  bool load(const std::string &filename, const CFileProvider &fp);
  float getrefresh();

  std::string gettype()
    { return std::string("AdLib Tracker 2"); }
  std::string gettitle()
    { return std::string(songname + 1, *songname); }
  std::string getauthor()
    { return std::string(author + 1, *author); }
  unsigned int getinstruments()
    { return NUMINST; }
  std::string getinstrument(unsigned int n)
  { return n < NUMINST ? std::string(instname[n] + 1, *instname[n]) : std::string(); }

private:
  enum {NUMINST = 250, INSTDATASIZE = 13};
  char songname[43], author[43], instname[NUMINST][33];
};

#endif
