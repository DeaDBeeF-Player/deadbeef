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
 * a2m.cpp - A2M Loader by Simon Peter <dn.tlp@gmx.net>
 *
 * NOTES:
 * This loader detects and loads version 1, 4, 5 & 8 files.
 *
 * version 1-4 files:
 * Following commands are ignored: FF1 - FF9, FAx - FEx
 *
 * version 5-8 files:
 * Instrument panning is ignored. Flags byte is ignored.
 * Following commands are ignored: Gxy, Hxy, Kxy - &xy
 */

#include <cstring>
#include <cassert>
#include "a2m.h"

// Limit length byte not to exceed size of the string
#define fixstringlength(s) __fixlength((s)[0], sizeof(s) - 1);
static void __fixlength(char &len, size_t max) {
  if ((unsigned char)len > max) len = (char)max;
}

CPlayer *Ca2mLoader::factory(Copl *newopl)
{
  return new Ca2mLoader(newopl);
}

bool Ca2mLoader::load(const std::string &filename, const CFileProvider &fp)
{
  binistream *f = fp.open(filename); if (!f) return false;
  unsigned int i;
  int j, k, t;
  unsigned int l;
  unsigned char *org, *orgptr, flags = 0;
  unsigned long alength;
  unsigned short len[9], *secdata, *secptr;
  static const unsigned char convfx[16] = {
	0, 1, 2, 23, 24, 3, 5, 4, 6, 9, 17, 13, 11, 19, 7, 14
  };
  static const unsigned char convinf1[16] = {
	0, 1, 2, 6, 7, 8, 9, 4, 5, 3, 10, 11, 12, 13, 14, 15
  };
  static const unsigned char newconvfx[] = {
	0, 1, 2, 3, 4, 5, 6, 23, 24, 21, 10, 11, 17, 13, 7, 19,
	255, 255, 22, 25, 255, 15, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 14, 255
  };

  // read header
  char id[10];
  f->readString(id, sizeof(id));
  /*unsigned long crc = */ f->readInt(4);
  unsigned char version = f->readInt(1);
  unsigned char numpats = f->readInt(1);

  // file validation section
  if (memcmp(id, "_A2module_", sizeof(id)) ||
      (version != 1 && version != 5 && version != 4 && version != 8) ||
      numpats < 1 || numpats > 64) {
    fp.close(f);
    return false;
  }

  nop = numpats;
  length = 128;
  restartpos = 0;

  // load, depack & convert section
  if (version < 5) {
    for (i = 0; i < 5; i++) len[i] = f->readInt(2);
    t = 9;
  } else {	// version >= 5
    for (i = 0; i < 9; i++) len[i] = f->readInt(2);
    t = 18;
  }

  // block 0
  size_t needed = sizeof(songname) + sizeof(author) + sizeof(instname)
    + NUMINST * INSTDATASIZE + length + 2 + (version >= 5);
  if (version == 1 || version == 5) {
    // needed bytes are used, so don't allocate or decode more than that.
    orgptr = org = new unsigned char [needed];
    secdata = new unsigned short [len[0] / 2];

    for (i = 0; i < len[0] / 2; i++) secdata[i] = f->readInt(2);

    // What if len[0] is odd: ignore, skip extra byte, or fail?
    l = Sixdepak::decode(secdata, len[0], org, needed);

    delete [] secdata;
  } else {
    orgptr = org = new unsigned char [len[0]];

    for(l = 0; l < len[0]; l++) orgptr[l] = f->readInt(1);
  }
  if (l < needed) {
    // Block is too short; fail.
    delete [] org;
    fp.close(f);
    return false;
  }

  memcpy(songname, orgptr, sizeof(songname));
  orgptr += sizeof(songname);
  fixstringlength(songname);
  memcpy(author, orgptr, sizeof(author));
  orgptr += sizeof(author);
  fixstringlength(author);
  memcpy(instname, orgptr, sizeof(instname));
  orgptr += sizeof(instname);

  for (i = 0; i < NUMINST; i++) {  // instrument data
    fixstringlength(instname[i]);

    inst[i].data[0] = orgptr[i * INSTDATASIZE + 10];
    inst[i].data[1] = orgptr[i * INSTDATASIZE + 0];
    inst[i].data[2] = orgptr[i * INSTDATASIZE + 1];
    inst[i].data[3] = orgptr[i * INSTDATASIZE + 4];
    inst[i].data[4] = orgptr[i * INSTDATASIZE + 5];
    inst[i].data[5] = orgptr[i * INSTDATASIZE + 6];
    inst[i].data[6] = orgptr[i * INSTDATASIZE + 7];
    inst[i].data[7] = orgptr[i * INSTDATASIZE + 8];
    inst[i].data[8] = orgptr[i * INSTDATASIZE + 9];
    inst[i].data[9] = orgptr[i * INSTDATASIZE + 2];
    inst[i].data[10] = orgptr[i * INSTDATASIZE + 3];

    if (version < 5)
      inst[i].misc = orgptr[i * INSTDATASIZE + 11];
    else {	// version >= 5 -> OPL3 format
      int pan = orgptr[i * INSTDATASIZE + 11];

      if (pan)
	inst[i].data[0] |= (pan & 3) << 4;	// set pan
      else
	inst[i].data[0] |= 0x30;		// enable both speakers
    }

    inst[i].slide = orgptr[i * INSTDATASIZE + 12];
  }
  orgptr += NUMINST * INSTDATASIZE;

  memcpy(order, orgptr, length);
  orgptr += length;
  for (i = 0; i < length; i++)
    if ((order[i] & 0x7f) >= numpats) {		// invalid pattern in order list
      // What to do in this case in not defined in documentation? There are songs that contain this fault.
      // We have two options:
      //  1. allocate an empty pattern and redirect to this - more work and lots of quirks
      //  2. adjust order list to point to pattern 0 / jump to order 0 instead
      order[i] &= 0x80; // make it point to pattern 0 / jump to order 0
    }

  bpm = *orgptr++;
  initspeed = *orgptr++;
  if (version >= 5) flags = *orgptr;

  delete [] org;

  // blocks 1-4 or 1-8
  unsigned char ppb = version < 5 ? 16 : 8; // patterns per block
  unsigned char blocks = (numpats + ppb - 1) / ppb; // excluding block 0
  alength = len[1];
  for (i = 2; i <= blocks; i++)
    alength += len[i];

  needed = numpats * 64 * t * 4;
  if (version == 1 || version == 5) {
    org = new unsigned char [needed];
    secdata = new unsigned short [alength / 2];

    for (l = 0; l < alength / 2; l++)
      secdata[l] = f->readInt(2);

    orgptr = org; secptr = secdata;
    for (i = 1; i <= blocks; i++) {
      orgptr += Sixdepak::decode(secptr, len[i], orgptr, org + needed - orgptr);
      secptr += len[i] / 2;
    }
    delete [] secdata;
  } else {
    org = new unsigned char [alength];
    f->readString((char *)org, alength);
    orgptr = org + alength;
  }

  if (orgptr - org < (signed long)needed) {
    delete [] org;
    fp.close(f);
    return false;
  }

  if (version < 5) {
    for (i = 0; i < numpats; i++)	// pattern
      for (j = 0; j < 64; j++)		// row
	for(k = 0; k < t /*9*/; k++) {	// channel
	  struct Tracks	*track = &tracks[i * t + k][j];
	  unsigned char	*o = &org[(i * 64 * t + j * t + k) * 4];

	  track->note = o[0] == 255 ? 127 : o[0];
	  track->inst = o[1] <= NUMINST ? o[1] : 0; // ignore invalid instrument
	  track->command = o[2] < sizeof(convfx) ? convfx[o[2]] : 255;
	  track->param2 = o[3] & 0x0f;
	  track->param1 = o[3] >> 4;

	  if (track->command == 14) {
	    track->param1 = convinf1[track->param1];
	    switch(track->param1) {
	    case 2: // convert define waveform
	      track->command = 25;
	      track->param1 = track->param2;
	      track->param2 = 0xf;
	      break;

	    case 8: // convert volume slide up
	      track->command = 26;
	      track->param1 = track->param2;
	      track->param2 = 0;
	      break;

	    case 9: // convert volume slide down
	      track->command = 26;
	      track->param1 = 0;
	      break;

	    case 15: // convert key-off
	      if(!track->param2) {
		track->command = 8;
		track->param1 = 0;
		// param2 is already zero
	      }
	      break;
	    }
	  }
	}
  } else {	// version >= 5
    realloc_patterns(numpats, 64, t);

    for (i = 0; i < numpats; i++)	// pattern
      for (j = 0; j < t /*18*/; j++)	// channel
	for (k = 0; k < 64; k++) {	// row
	  struct Tracks	*track = &tracks[i * t + j][k];
	  unsigned char	*o = &org[(i * 64 * t + j * 64 + k) * 4];

	  track->note = o[0] == 255 ? 127 : o[0];
	  track->inst = o[1] <= NUMINST ? o[1] : 0; // ignore invalid instrument
	  track->command = o[2] < sizeof(newconvfx) ? newconvfx[o[2]] : 255;
	  track->param1 = o[3] >> 4;
	  track->param2 = o[3] & 0x0f;

	  // Convert '&' command
	  if (o[2] == 36)
	    switch(track->param1) {
	    case 0:	// pattern delay (frames)
	      track->command = 29;
	      track->param1 = 0;
	      // param2 already set correctly
	      break;

	    case 1:	// pattern delay (rows)
	      track->command = 14;
	      track->param1 = 8;
	      // param2 already set correctly
	      break;
	    }
	}
  }

  init_trackord();

  delete [] org;

  // Process flags
  if (version >= 5) {
    CmodPlayer::flags |= Opl3;				// All versions >= 5 are OPL3
    if (flags & 8) CmodPlayer::flags |= Tremolo;	// Tremolo depth
    if (flags & 16) CmodPlayer::flags |= Vibrato;	// Vibrato depth
  }

  // Note: crc value is not checked.

  fp.close(f);
  rewind(0);
  return true;
}

float Ca2mLoader::getrefresh()
{
	if (tempo == 18)
		return 18.2f;
	else
		return (float) (tempo);
}
