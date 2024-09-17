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
 * [xad] HYP player, by Riven the Mage <riven@ok.ru>
 */

#include "xad.h"

class CxadhypPlayer: public CxadPlayer
{
public:
  static CPlayer *factory(Copl *newopl);

  CxadhypPlayer(Copl *newopl): CxadPlayer(newopl)
    { }

protected:
  struct
  {
    unsigned short  pointer;
  } hyp;
  //
  bool		    xadplayer_load()
    {
      if (xad.fmt == HYP && tune_size >= 0x69 + 9)
	return true;
      else
	return false;
    }
  void 		    xadplayer_rewind(int subsong);
  void 		    xadplayer_update();
  float 	    xadplayer_getrefresh();
  std::string	    xadplayer_gettype();

private:
  static const unsigned char hyp_adlib_registers[99];
  static const unsigned short hyp_notes[73];
};
