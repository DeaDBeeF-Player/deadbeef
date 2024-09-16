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
 * [xad] FLASH player, by Riven the Mage <riven@ok.ru>
 */

#include "xad.h"

class CxadflashPlayer: public CxadPlayer
{
public:
  static CPlayer *factory(Copl *newopl);

  CxadflashPlayer(Copl *newopl): CxadPlayer(newopl)
    { };

protected:
  struct
  {
    unsigned char   order_pos;
    unsigned char   pattern_pos;
  } flash;
  //
  bool		  xadplayer_load()
    {
      if (xad.fmt == FLASH && tune_size >= 0x633 + 18)
	return true;
      else
	return false;
    }
  void            xadplayer_rewind(int subsong);
  void            xadplayer_update();
  float           xadplayer_getrefresh();
  std::string     xadplayer_gettype();
  unsigned int    xadplayer_getinstruments();
};
