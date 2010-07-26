/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2006 Simon Peter, <dn.tlp@gmx.net>, et al.
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
 * mtk.h - MPU-401 Trakker Loader by Simon Peter <dn.tlp@gmx.net>
 */

#include "hsc.h"

class CmtkLoader: public ChscPlayer
{
 public:
  static CPlayer *factory(Copl *newopl);

  CmtkLoader(Copl *newopl)
    : ChscPlayer(newopl)
    {
      mtkmode = 1;
    };

  bool load(const char *filename, const CFileProvider &fp);

  const char * gettype()
    { return "MPU-401 Trakker"; };
  const char * gettitle()
    { return title; };
  const char * getauthor()
    { return composer; };
  unsigned int getinstruments()
    { return 128; };
  const char * getinstrument(unsigned int n)
    { return instname[n]; };

 private:
  char title[34],composer[34],instname[0x80][34];
};
