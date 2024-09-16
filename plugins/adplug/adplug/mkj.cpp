/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2004 Simon Peter, <dn.tlp@gmx.net>, et al.
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
 * mkj.cpp - MKJamz Player, by Simon Peter <dn.tlp@gmx.net>
 */

#include <cstring>
#include <assert.h>

#include "mkj.h"
#include "debug.h"

CPlayer *CmkjPlayer::factory(Copl *newopl)
{
  return new CmkjPlayer(newopl);
}

bool CmkjPlayer::load(const std::string &filename, const CFileProvider &fp)
{
  binistream *f = fp.open(filename); if(!f) return false;
  char	id[6];
  float	ver;
  int	i, j;

  // file validation
  f->readString(id, 6);
  if(strncmp(id,"MKJamz",6)) { fp.close(f); return false; }
  ver = f->readFloat(binio::Single);
  if(ver > 1.12) { fp.close(f); return false; }

  // load
  maxchannel = f->readInt(2);
  if (maxchannel > 9 || maxchannel < 0) {
    fp.close(f);
    return false;
  }
  // load and store the channel instruments.
  for(i = 0; i < maxchannel; i++) {
	for (j = 0; j < 8; j++) {
	  inst[i].value[j] = f->readInt(2);
	}
  }
  maxnotes = f->readInt(2);
  if (maxnotes < 1 ||
      // Larger values would need code changes to avoid integer overflows:
      maxnotes > 0x7fff / (maxchannel + 1) ||
      // That's what gets actually accessed in update(): [yeah, it's weird]
      maxnotes - 1 + 3 * maxchannel > (maxchannel + 1) * maxnotes) {
    fp.close(f);
    return false;
  }
  delete[] songbuf;
  songbuf = new short [(maxchannel+1)*maxnotes];
  for(i = 0; i < maxchannel; i++) channel[i].defined = f->readInt(2);
  for(i = 0; i < (maxchannel + 1) * maxnotes; i++)
    songbuf[i] = f->readInt(2);

  if (f->error()) {
    fp.close(f);
    return false;
  }
  AdPlug_LogWrite("CmkjPlayer::load(\"%s\"): loaded file ver %.2f, %d channels,"
		  " %d notes/channel.\n", filename.c_str(), ver, maxchannel,
		  maxnotes);
  fp.close(f);
  rewind(0);
  return true;
}

bool CmkjPlayer::update()
{
  int c, i;
  short note;

  for(c = 0; c < maxchannel; c++) {
    if(!channel[c].defined)	// skip if channel is disabled
      continue;

    if(channel[c].pstat) {
      channel[c].pstat--;
      continue;
    }

    opl->write(0xb0 + c, 0);	// key off
    do {
      assert(channel[c].songptr < (maxchannel + 1) * maxnotes);
      note = songbuf[channel[c].songptr];
      if(channel[c].songptr - c > maxchannel)
	if(note && note < 250)
	  channel[c].pstat = channel[c].speed;
      switch(note) {
	// normal notes
      case 68: opl->write(0xa0 + c,0x81); opl->write(0xb0 + c,0x21 + 4 * channel[c].octave); break;
      case 69: opl->write(0xa0 + c,0xb0); opl->write(0xb0 + c,0x21 + 4 * channel[c].octave); break;
      case 70: opl->write(0xa0 + c,0xca); opl->write(0xb0 + c,0x21 + 4 * channel[c].octave); break;
      case 71: opl->write(0xa0 + c,0x2); opl->write(0xb0 + c,0x22 + 4 * channel[c].octave); break;
      case 65: opl->write(0xa0 + c,0x41); opl->write(0xb0 + c,0x22 + 4 * channel[c].octave); break;
      case 66: opl->write(0xa0 + c,0x87); opl->write(0xb0 + c,0x22 + 4 * channel[c].octave); break;
      case 67: opl->write(0xa0 + c,0xae); opl->write(0xb0 + c,0x22 + 4 * channel[c].octave); break;
      case 17: opl->write(0xa0 + c,0x6b); opl->write(0xb0 + c,0x21 + 4 * channel[c].octave); break;
      case 18: opl->write(0xa0 + c,0x98); opl->write(0xb0 + c,0x21 + 4 * channel[c].octave); break;
      case 20: opl->write(0xa0 + c,0xe5); opl->write(0xb0 + c,0x21 + 4 * channel[c].octave); break;
      case 21: opl->write(0xa0 + c,0x20); opl->write(0xb0 + c,0x22 + 4 * channel[c].octave); break;
      case 15: opl->write(0xa0 + c,0x63); opl->write(0xb0 + c,0x22 + 4 * channel[c].octave); break;
      case 255:	// delay
	channel[c].songptr += maxchannel;
        if (songbuf[channel[c].songptr] < 0)
          goto bad_data; // avoid integer overflow
	channel[c].pstat = songbuf[channel[c].songptr];
	break;
      case 254:	// set octave
	channel[c].songptr += maxchannel;
        if ((songbuf[channel[c].songptr] | 7) != 7)
          goto bad_data; // value out of range
	channel[c].octave = songbuf[channel[c].songptr];
	break;
      case 253:	// set speed
	channel[c].songptr += maxchannel;
        if (songbuf[channel[c].songptr] < 0)
          goto bad_data; // avoid integer overflow
	channel[c].speed = songbuf[channel[c].songptr];
	break;
      case 252:	// set waveform
	channel[c].songptr += maxchannel;
        if (((songbuf[channel[c].songptr] - 300) | 0xff) != 0xff)
          goto bad_data; // value out of range
	channel[c].waveform = songbuf[channel[c].songptr] - 300;
	if(c > 2)
	  opl->write(0xe0 + c + (c+6),channel[c].waveform);
	else
	  opl->write(0xe0 + c,channel[c].waveform);
	break;
      case 251:	// song end
      bad_data:
	for(i = 0; i < maxchannel; i++) channel[i].songptr = i;
	songend = true;
	return false;
      }

      if(channel[c].songptr - c < maxnotes)
	channel[c].songptr += maxchannel;
      else
	channel[c].songptr = c;
    } while(!channel[c].pstat);
  }

  return !songend;
}

void CmkjPlayer::rewind(int subsong)
{
  int i;

  opl->init(); opl->write(1, 32);
  for(i = 0; i < maxchannel; i++) {
    channel[i].pstat = 0;
    channel[i].speed = 0;
    channel[i].waveform = 0;
    channel[i].songptr = i;
    channel[i].octave = 4;
    // Set up channel instruments
    opl->write(0x20+op_table[i],inst[i].value[4]);
    opl->write(0x23+op_table[i],inst[i].value[0]);
    opl->write(0x40+op_table[i],inst[i].value[5]);
    opl->write(0x43+op_table[i],inst[i].value[1]);
    opl->write(0x60+op_table[i],inst[i].value[6]);
    opl->write(0x63+op_table[i],inst[i].value[2]);
    opl->write(0x80+op_table[i],inst[i].value[7]);
    opl->write(0x83+op_table[i],inst[i].value[3]);
  }

  songend = false;
}

float CmkjPlayer::getrefresh()
{
  return 100.0f;
}
