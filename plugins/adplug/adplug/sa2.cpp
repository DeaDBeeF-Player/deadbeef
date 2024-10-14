/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2008 Simon Peter, <dn.tlp@gmx.net>, et al.
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
 * sa2.cpp - SAdT2 Loader by Simon Peter <dn.tlp@gmx.net>
 *           SAdT Loader by Mamiya <mamiya@users.sourceforge.net>
 */

#include <cstring>
#include <cstdio>

#include "sa2.h"
#include "debug.h"

CPlayer *Csa2Loader::factory(Copl *newopl)
{
  return new Csa2Loader(newopl);
}

bool Csa2Loader::load(const std::string &filename, const CFileProvider &fp)
{
  binistream *f = fp.open(filename); if(!f) return false;
  unsigned char buf;
  unsigned int i,j, k, notedis = 0;
  static const unsigned char convfx[16] = {
    0, 1, 2, 3, 4, 5, 6, 255, 8, 255, 10, 11, 12, 13, 255, 15
  };
  unsigned char sat_type;
  enum SAT_TYPE {
    HAS_ARPEGGIOLIST = (1 << 7),
    HAS_V7PATTERNS = (1 << 6),
    HAS_ACTIVECHANNELS = (1 << 5),
    HAS_TRACKORDER = (1 << 4),
    HAS_ARPEGGIO = (1 << 3),
    HAS_OLDBPM = (1 << 2),
    HAS_OLDPATTERNS = (1 << 1),
    HAS_UNKNOWN127 = (1 << 0)
  };

  // read header
  f->readString(header.sadt, 4);
  header.version = f->readInt(1);

  // file validation section
  if(memcmp(header.sadt, "SAdT", 4)) {
    fp.close(f);
    return false;
  }
  switch(header.version) {
  case 1:
    notedis = +0x18;
    sat_type = HAS_UNKNOWN127 | HAS_OLDPATTERNS | HAS_OLDBPM;
    break;
  case 2:
    notedis = +0x18;
    sat_type = HAS_OLDPATTERNS | HAS_OLDBPM;
    break;
  case 3:
    notedis = +0x0c;
    sat_type = HAS_OLDPATTERNS | HAS_OLDBPM;
    break;
  case 4:
    notedis = +0x0c;
    sat_type = HAS_ARPEGGIO | HAS_OLDPATTERNS | HAS_OLDBPM;
    break;
  case 5:
    notedis = +0x0c;
    sat_type = HAS_ARPEGGIO | HAS_ARPEGGIOLIST | HAS_OLDPATTERNS | HAS_OLDBPM;
    break;
  case 6:
    sat_type = HAS_ARPEGGIO | HAS_ARPEGGIOLIST | HAS_OLDPATTERNS | HAS_OLDBPM;
    break;
  case 7:
    sat_type = HAS_ARPEGGIO | HAS_ARPEGGIOLIST | HAS_V7PATTERNS;
    break;
  case 8:
    sat_type = HAS_ARPEGGIO | HAS_ARPEGGIOLIST | HAS_TRACKORDER;
    break;
  case 9:
    sat_type = HAS_ARPEGGIO | HAS_ARPEGGIOLIST | HAS_TRACKORDER | HAS_ACTIVECHANNELS;
    break;
  default:	/* unknown */
    fp.close(f);
    return false;
  }

  // load section
  // instruments
  for (i = 0; i < 31; i++) {
    for (j = 0; j < 11; j++)
      inst[i].data[j] = f->readInt(1);
    if (sat_type & HAS_ARPEGGIO) {
      inst[i].arpstart = f->readInt(1);
      inst[i].arpspeed = f->readInt(1);
      inst[i].arppos = f->readInt(1);
      inst[i].arpspdcnt = f->readInt(1);
    } else {
      inst[i].arpstart = 0;
      inst[i].arpspeed = 0;
      inst[i].arppos = 0;
      inst[i].arpspdcnt = 0;
    }
    inst[i].misc = 0;
    inst[i].slide = 0;
  }

  // instrument names
  for(i = 0; i < 29; i++) f->readString(instname[i], 17);

  f->ignore(3);		// dummy bytes
  for(i = 0; i < 128; i++) order[i] = f->readInt(1);	// pattern orders
  if(sat_type & HAS_UNKNOWN127) f->ignore(127);

  // infos
  nop = f->readInt(2); length = f->readInt(1); restartpos = f->readInt(1);
  // checks
  if (nop < 1 || nop > 64 ||
      length < 1 || length > 128 ||
      restartpos >= length) {
    fp.close(f);
    return false;
  }
  for (i = 0; i < length; i++)	// check order
    if (order[i] >= nop /* or 64 */) {
      fp.close(f);
      return false;
    }

  // bpm
  bpm = f->readInt(2);
  if(sat_type & HAS_OLDBPM) {
    bpm = bpm * 125 / 50;		// cps -> bpm
  }

  if(sat_type & HAS_ARPEGGIOLIST) {
    init_specialarp();
    for(i = 0; i < 256; i++) arplist[i] = f->readInt(1);	// arpeggio list
    for(i = 0; i < 256; i++) arpcmd[i] = f->readInt(1);	// arpeggio commands
  }

  for (i = 0; i < 64; i++) {				// track orders
    for (j = 0; j < 9; j++) {
      if (sat_type & HAS_TRACKORDER) {
	// the value read should be < 9 * nop, but can't cause invalid accesses
	trackord[i][j] = f->readInt(1);
      } else {
	trackord[i][j] = i * 9 + j; // + 1 ???
      }
    }
  }

  if(sat_type & HAS_ACTIVECHANNELS)
    activechan = f->readInt(2) << 16;		// active channels

  AdPlug_LogWrite("Csa2Loader::load(\"%s\"): sat_type = %x, nop = %d, "
		  "length = %d, restartpos = %d, activechan = %x, bpm = %d\n",
		  filename.c_str(), sat_type, nop, length, restartpos, activechan, bpm);

  // track data
  if(sat_type & HAS_OLDPATTERNS) {
    i = 0;
    while (i < 64 * 9 && !f->ateof()) {
      for(j=0;j<64;j++) {
	for(k=0;k<9;k++) {
	  buf = f->readInt(1);
	  tracks[i+k][j].note = buf ? (buf + notedis) : 0;
	  tracks[i+k][j].inst = f->readInt(1);
	  tracks[i+k][j].command = convfx[f->readInt(1) & 0xf];
	  tracks[i+k][j].param1 = f->readInt(1);
	  tracks[i+k][j].param2 = f->readInt(1);
	}
      }
      i+=9;
    }
  } else
    if(sat_type & HAS_V7PATTERNS) {
      i = 0;
      while (i < 64 * 9 && !f->ateof()) {
	for(j=0;j<64;j++) {
	  for(k=0;k<9;k++) {
	    buf = f->readInt(1);
	    tracks[i+k][j].note = buf >> 1;
	    tracks[i+k][j].inst = (buf & 1) << 4;
	    buf = f->readInt(1);
	    tracks[i+k][j].inst += buf >> 4;
	    tracks[i+k][j].command = convfx[buf & 0x0f];
	    buf = f->readInt(1);
	    tracks[i+k][j].param1 = buf >> 4;
	    tracks[i+k][j].param2 = buf & 0x0f;
	  }
	}
	i+=9;
      }
    } else {
      i = 0;
      while (i < 64 * 9 && !f->ateof()) {
	for(j=0;j<64;j++) {
	  buf = f->readInt(1);
	  tracks[i][j].note = buf >> 1;
	  tracks[i][j].inst = (buf & 1) << 4;
	  buf = f->readInt(1);
	  tracks[i][j].inst += buf >> 4;
	  tracks[i][j].command = convfx[buf & 0x0f];
	  buf = f->readInt(1);
	  tracks[i][j].param1 = buf >> 4;
	  tracks[i][j].param2 = buf & 0x0f;
	}
	i++;
      }
    }
  fp.close(f);

  // fix instrument names
  for(i=0;i<29;i++)
    for(j=0;j<17;j++)
      if(!instname[i][j])
	instname[i][j] = ' ';

  rewind(0);		// rewind module
  return true;
}

std::string Csa2Loader::gettype()
{
  char tmpstr[40];
  snprintf(tmpstr,sizeof(tmpstr),"Surprise! Adlib Tracker 2 (version %d)",header.version);
  return std::string(tmpstr);
}

std::string Csa2Loader::gettitle()
{
  char buf[29 * 17 - 1];
  int i, j, len = 0, ptr = 0, spaces = 0;

  // parse instrument names for song name

  for (i = 0; i < 29; i++)
    for (j = 1; j < 17; j++)
      if (instname[i][j] == '"') goto title_start;

  return std::string();

  for (/*i = 0*/; i < 29; i++) {
    for (j = 1; j < 17; j++) {
      if (instname[i][j] == ' ')
	spaces++;
      else
	spaces = 0;
      if (instname[i][j] == '"')
	len = ptr;
      buf[ptr++] = instname[i][j];
    title_start:;
    }
    ptr -= spaces;
    spaces = 1;
    buf[ptr++] = ' ';
  }

  return std::string(buf, len);
}
