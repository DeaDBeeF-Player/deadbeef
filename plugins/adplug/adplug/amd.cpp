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
 * amd.cpp - AMD Loader by Simon Peter <dn.tlp@gmx.net>
 */

#include <algorithm>
#include <cstring>

#include "amd.h"
#include "debug.h"

CPlayer *CamdLoader::factory(Copl *newopl)
{
  return new CamdLoader(newopl);
}

bool CamdLoader::load(const std::string &filename, const CFileProvider &fp)
{
  binistream *f = fp.open(filename);
  if (!f) return false;

  // file validation section
  struct {
    char id[9];
    unsigned char version;
    enum {offset = 1062};
  } header;

  if (fp.filesize(f) < header.offset + sizeof(header)) {
    fp.close(f);
    return false;
  }
  f->seek(header.offset);
  f->readString(header.id, sizeof(header.id));
  if (memcmp(header.id, "<o\xefQU\xeeRoR", sizeof(header.id)) &&
      memcmp(header.id, "MaDoKaN96", sizeof(header.id))) {
    fp.close(f);
    return false;
  }
  header.version = f->readInt(1);

  // load section
  f->seek(0);
  // title + author
  f->readString(songname, sizeof(songname));
  f->readString(author, sizeof(author));

  // instruments
  for (size_t i = 0; i < 26; i++) {
    f->readString(instname[i], sizeof(instname[i])); // load name
    for (size_t j = 0; j < sizeof(instname[i]); j++) // convert name
      if (instname[i][j] == '\xff')
	instname[i][j] = '\x20';

    inst[i].data[1] =  f->readInt(1); // load & reorder data
    inst[i].data[9] =  f->readInt(1);
    inst[i].data[3] =  f->readInt(1);
    inst[i].data[5] =  f->readInt(1);
    inst[i].data[7] =  f->readInt(1);
    inst[i].data[2] =  f->readInt(1);
    inst[i].data[10] =  f->readInt(1);
    inst[i].data[4] =  f->readInt(1);
    inst[i].data[6] =  f->readInt(1);
    inst[i].data[8] =  f->readInt(1);
    inst[i].data[0] =  f->readInt(1);
  }

  // order length + # of patterns
  length = f->readInt(1);
  nop = f->readInt(1) + 1;
  if (length > 128 || nop > 64) {
    fp.close(f);
    return false;
  }

  // order list
  f->readString((char *)order, 128);
  // invalid pattern number in order list?
  for (size_t i = 0; i < length; i++)
    if ((order[i] & 0x7f) >= 64) { // should be < nop
      fp.close(f);
      return false;
    }

  f->ignore(10);

  // track data
  int maxi = 0;
  if (header.version == 0x10) {	// unpacked module
    init_trackord();
    maxi = nop * 9;

    for (int t = 0; t < maxi && !f->ateof(); t += 9) {
      for (int j = 0; j < 64; j++)
	for (int i = t; i < t + 9; i++) {
	  unsigned char buf = f->readInt(1) & 0x7f;
	  tracks[i][j].param1 = buf / 10;
	  tracks[i][j].param2 = buf % 10;

	  buf = f->readInt(1);
	  tracks[i][j].command = buf & 0x0f;
	  tracks[i][j].inst = buf >> 4;

	  buf = f->readInt(1);
	  tracks[i][j].inst += (buf & 1) << 4;
	  if(buf >> 4)	// fix bug in AMD save routine
	    tracks[i][j].note = ((buf & 0x0e) >> 1) * 12 + (buf >> 4);
	  else
	    tracks[i][j].note = 0;
	}
    }
  } else {			// packed module
    for (int i = 0; i < nop; i++)
      for (int j = 0; j < 9; j++) {
	trackord[i][j] = f->readInt(2) + 1;
	if (trackord[i][j] > 64 * 9) trackord[i][j] = 0; // or fail?
      }

    int numtrax = f->readInt(2);
    for (int k = 0; k < numtrax; k++) {
      int i = std::min((int)f->readInt(2), 64 * 9 - 1); // fix corrupted modules
      maxi = std::max(i + 1, maxi);

      for (int j = 0; j < 64; j++) {
	unsigned char buf = f->readInt(1);

	if (buf & 0x80) { // packed block of empty events
	  int len = std::min(buf & 0x7f, 64 - j);
	  memset(&tracks[i][j], 0, len * sizeof(tracks[i][j]));
	  j += len - 1; // adjust for increment in loop header
	  continue;
	}

	// normal data, same as above
	tracks[i][j].param1 = buf / 10;
	tracks[i][j].param2 = buf % 10;

	buf = f->readInt(1);
	tracks[i][j].command = buf & 0x0f;
	tracks[i][j].inst = buf >> 4;

	buf = f->readInt(1);
	tracks[i][j].inst += (buf & 1) << 4;
	if(buf >> 4)	// fix bug in AMD save routine
	  tracks[i][j].note = ((buf & 0x0e) >> 1) * 12 + (buf >> 4);
	else
	  tracks[i][j].note = 0;
      }
    }
  }
  fp.close(f);

  bpm = 50;
  restartpos = 0;
  flags = Decimal;
  // convert patterns to protracker replay data
  for (int i = 0; i < maxi; i++)
    for (int j = 0; j < 64; j++) {
      static const unsigned char convfx[] = {0, 1, 2, 9, 17, 11, 13, 18, 3, 14};
      if (tracks[i][j].command < sizeof(convfx))
	tracks[i][j].command = convfx[tracks[i][j].command];
      else
	tracks[i][j].command = 0; // ignore invalid commands

      // extended command
      if (tracks[i][j].command == 14) {
	if (tracks[i][j].param1 == 2) {
	  tracks[i][j].command = 10;
	  tracks[i][j].param1 = tracks[i][j].param2;
	  tracks[i][j].param2 = 0;
	}

	if (tracks[i][j].param1 == 3) {
	  tracks[i][j].command = 10;
	  tracks[i][j].param1 = 0;
	}
      }

      // fix volume
      if (tracks[i][j].command == 17) {
	unsigned int vol = tracks[i][j].param1 * 10 + tracks[i][j].param2;
	static const unsigned char convvol[64] = {
	  0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03,
	  0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07,
	  0x07, 0x08, 0x09, 0x09, 0x0a, 0x0a, 0x0b, 0x0c,
	  0x0c, 0x0d, 0x0e, 0x0e, 0x0f, 0x10, 0x10, 0x11,
	  0x12, 0x13, 0x14, 0x14, 0x15, 0x16, 0x17, 0x18,
	  0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x21,
	  0x22, 0x23, 0x25, 0x26, 0x28, 0x29, 0x2b, 0x2d,
	  0x2e, 0x30, 0x32, 0x35, 0x37, 0x3a, 0x3c, 0x3f
	};

	if (vol < sizeof(convvol)) vol = convvol[vol];
	else vol = 63;

	tracks[i][j].param1 = vol / 10;
	tracks[i][j].param2 = vol % 10;
      }
    }

  rewind(0);
  return true;
}

float CamdLoader::getrefresh()
{
  if (tempo)
    return (float)tempo;
  else
    return 18.2f;
}
