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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * mtk.cpp - MPU-401 Trakker Loader by Simon Peter (dn.tlp@gmx.net)
 */

#include <cstring>
#include "mtk.h"

/*** public methods **************************************/

CPlayer *CmtkLoader::factory(Copl *newopl)
{
  return new CmtkLoader(newopl);
}

bool CmtkLoader::load(const std::string &filename, const CFileProvider &fp)
{
  binistream *f = fp.open(filename);
  if (!f) return false;

  struct {
    char id[18];
    unsigned short crc, size;
  } header;
  struct mtkdata {
    struct { char dummy, str[33]; } songname, composername, instname[0x80];
    unsigned char insts[0x80][12], order[0x80], dummy;
    // followed by pattern data:
    // hscnote patterns[50][64*9];
  } *data;
  unsigned int i, cnt;

  // read header
  f->readString(header.id, sizeof(header.id));
  header.crc = f->readInt(2);
  header.size = f->readInt(2);

  // file validation section
  if (memcmp(header.id, "mpu401tr\x92kk\xeer@data", sizeof(header.id)) ||
      header.size < sizeof(*data)) {
    fp.close(f); return false;
  }

  // load & decompress section
  unsigned short ctrlbits = 0, ctrlmask = 0;
  unsigned char *org = new unsigned char[header.size];
  for (size_t orgptr = 0; orgptr < header.size; orgptr += cnt) {
    if (f->error()) goto err;

    ctrlmask >>= 1;
    if (!ctrlmask) {
      ctrlbits = f->readInt(2);
      ctrlmask = 0x8000;
    }

    if (!(ctrlbits & ctrlmask)) {	// uncompressed data
      org[orgptr] = f->readInt(1);
      cnt = 1;
      continue;
    }

    // compressed data
    unsigned offs;
    unsigned char cmd = f->readInt(1);
    cnt = (cmd & 0x0f) + 3;

    switch (cmd >> 4) {
    case 0:	// repeat a byte 3..18 times
    repeat_byte:
      if (orgptr + cnt > header.size) goto err;
      memset(&org[orgptr], f->readInt(1), cnt);
      break;

    case 1:	// repeat a byte 19..4114 times
      cnt += (f->readInt(1) << 4) + 16;
      goto repeat_byte;

    case 2:	// copy range (16..271 bytes)
      offs = cnt + (f->readInt(1) << 4);
      cnt = f->readInt(1) + 16;
    copy_range:
      if (orgptr + cnt > header.size || offs > orgptr) goto err;
      // may overlap, can't use memcpy()
      for (i = 0; i < cnt; i++)
        org[orgptr + i] = org[orgptr - offs + i];
      break;

    default:	// copy range (3..15 bytes)
      offs = cnt + (f->readInt(1) << 4);
      cnt = cmd >> 4;
      goto copy_range;
    }
  }
  if (f->error() || !f->ateof()) goto err;
  fp.close(f);

  // convert to HSC replay data
  data = (struct mtkdata *) org;
  strncpy(title, data->songname.str, sizeof(title) - 1);
  title[sizeof(title) - 1] = 0;
  strncpy(composer, data->composername.str, sizeof(composer) - 1);
  composer[sizeof(composer) - 1] = 0;

  for (i = 0; i < 0x80; i++) {
    strncpy(instname[i], data->instname[i].str, sizeof(instname[i]) - 1);
    instname[i][sizeof(instname[i]) - 1] = 0;
  }

  memcpy(instr, data->insts, sizeof(instr));
  for (i = 0; i < 0x80; i++) {		// correct instruments
    instr[i][2] ^= (instr[i][2] & 0x40) << 1;
    instr[i][3] ^= (instr[i][3] & 0x40) << 1;
    instr[i][11] >>= 4;		// make unsigned
  }

  memcpy(song, data->order, sizeof(song));

  cnt = header.size - sizeof(*data); // was off by 1
  if (cnt > sizeof(patterns)) cnt = sizeof(patterns); // fail?
  memcpy(patterns, org + sizeof(*data), cnt);

  delete [] org;
  rewind(0);
  return true;

 err:
  fp.close(f);
  delete [] org;
  return false;
}
