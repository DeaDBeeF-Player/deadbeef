/*
  Adplug - Replayer for many OPL2/OPL3 audio file formats.
  Copyright (C) 1999 - 2003 Simon Peter, <dn.tlp@gmx.net>, et al.

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

  mtr.h - MTR loader by Dmitry Smagin <dmitry.s.smagin@gmail.com>
*/

#include "protrack.h"

class CmtrLoader : public CmodPlayer {
public:
    static CPlayer *factory(Copl *newopl);

    CmtrLoader(Copl *newopl) : CmodPlayer(newopl){};

    bool load(const std::string &filename, const CFileProvider &fp);
    float getrefresh();

    std::string gettype();
    std::string getinstrument(unsigned int n);
    unsigned int getinstruments();
    std::string gettitle();

private:
    struct mtr_instrument {
        char name[21];
        char is_used; // 2 - used, 0 unused
        char data[12];
    } instruments[64];

    unsigned char timer;
    unsigned int version, ninstruments;
    std::string title;
};
