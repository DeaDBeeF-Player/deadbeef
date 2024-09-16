/*
  Adplug - Replayer for many OPL2/OPL3 audio file formats.
  Copyright (C) 1999 - 2006 Simon Peter, <dn.tlp@gmx.net>, et al.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

  dmo.cpp - TwinTeam loader by Riven the Mage <riven@ok.ru>
*/

#include <stdint.h>

#include "s3m.h"

class CdmoLoader: public Cs3mPlayer
{
 public:
  static CPlayer *factory(Copl *newopl);

  CdmoLoader(Copl *newopl) : Cs3mPlayer(newopl) { };

  bool	load(const std::string &filename, const CFileProvider &fp);

  std::string	gettype();
  std::string	getauthor();

 private:

  class dmo_unpacker {
  public:
    enum { headersize = 12 };
    bool decrypt(unsigned char *buf, size_t len);
    static size_t unpack(unsigned char *ibuf, size_t inputsize,
			 unsigned char *obuf, size_t outputsize);

  private:
    unsigned short brand(unsigned short range);
    static long unpack_block(unsigned char *ibuf, size_t ilen,
			unsigned char *obuf, size_t olen);

    uint32_t bseed;
  };
};
