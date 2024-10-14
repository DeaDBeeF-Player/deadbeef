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

  dtm.h - DTM loader by Riven the Mage <riven@ok.ru>
*/

#include "protrack.h"

class CdtmLoader: public CmodPlayer
{
 public:
  static CPlayer *factory(Copl *newopl);

  CdtmLoader(Copl *newopl) : CmodPlayer(newopl) { };

  bool	load(const std::string &filename, const CFileProvider &fp);
  void	rewind(int subsong);
  float	getrefresh();

  std::string     gettype();
  std::string     gettitle();
  std::string     getauthor();
  std::string     getdesc();
  std::string     getinstrument(unsigned int n);
  unsigned int    getinstruments();

 private:
  enum {
    N_CHAN = 9,
    N_ROW = 64,
    N_ORD = 100,
    MAX_INST = 128,
    DESC_COLS = 80,
    DESC_ROWS = 16,
    DESC_SIZE = DESC_COLS * DESC_ROWS,
  };

  struct dtm_header
  {
    char            id[12];
    unsigned char   version;
    char            title[20];
    char            author[20];
    unsigned char   numpat;
    unsigned char   numinst;
  } header;

  char desc[DESC_SIZE];

  struct dtm_instrument
  {
    char            name[13];
    unsigned char   data[12];
  } instruments[MAX_INST];

  struct dtm_event
  {
    unsigned char	byte0;
    unsigned char	byte1;
  };

  bool unpack_pattern(binistream *f, size_t ilen, void *obuf, size_t olen);
};
