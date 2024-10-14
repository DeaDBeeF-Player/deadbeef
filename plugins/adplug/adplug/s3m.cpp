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
 * s3m.c - S3M Player by Simon Peter <dn.tlp@gmx.net>
 *
 * BUGS:
 * Extra Fine Slides (EEx, FEx) & Fine Vibrato (Uxy) are inaccurate
 */

#include <algorithm>
#include <cstring>
#include "s3m.h"
#include "debug.h"

// S3M -> adlib channel conversion
static const signed char chnresolv[32] = {
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   0,  1,  2,  3,  4,  5,  6,  7,  8, -1, -1, -1, -1, -1, -1, -1
};

// S3M adlib note table
static const unsigned short notetable[12] = {
  340, 363, 385, 408, 432, 458, 485, 514, 544, 577, 611, 647
};

// vibrato rate table
static const unsigned char vibratotab[32] = {
   1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16,
  16, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1
};

/*** public methods *************************************/

CPlayer *Cs3mPlayer::factory(Copl *newopl)
{
  return new Cs3mPlayer(newopl);
}

Cs3mPlayer::Cs3mPlayer(Copl *newopl): CPlayer(newopl)
{
  memset(orders, 0xff, sizeof(orders));
  memset(pattern, 0xff, sizeof(pattern));

  for (int i = 0; i < 99; i++)		// setup pattern
    for (int j = 0; j < 64; j++)
      for (int k = 0; k < 32; k++) {
	pattern[i][j][k].instrument = 0;
	pattern[i][j][k].info = 0;
      }
}

bool Cs3mPlayer::load(const std::string &filename, const CFileProvider &fp)
{
  binistream		*f = fp.open(filename); if(!f) return false;
  unsigned short	insptr[99],pattptr[99];
  int			i;

  // load header
  f->readString(header.name, 28);
  header.id = f->readInt(1);
  header.type = f->readInt(1);
  f->ignore(2);
  header.ordnum = f->readInt(2);
  header.insnum = f->readInt(2);
  header.patnum = f->readInt(2);
  header.flags = f->readInt(2);
  header.cwtv = f->readInt(2);
  header.ffi = f->readInt(2);
  f->readString(header.scrm, 4);
  header.gv = f->readInt(1);
  header.is = f->readInt(1);
  header.it = f->readInt(1);
  header.mv = f->readInt(1);
  header.uc = f->readInt(1);
  header.dp = f->readInt(1);
  f->ignore(8);
  header.special = f->readInt(2);
  for (i = 0; i < 32; i++)
    header.chanset[i] = f->readInt(1);

  // validate header
  if (header.id != 0x1a || header.type != 16 ||
      memcmp(header.scrm, "SCRM", 4) ||
      header.ordnum > 256 || header.insnum > 99 || header.patnum > 99) {
    fp.close(f);
    return false;
  }

  // load section
  for (i = 0; i < header.ordnum; i++)	// read orders
    orders[i] = f->readInt(1);
  for (i = 0; i < header.insnum; i++)	// instrument offsets
    insptr[i] = f->readInt(2);
  for (i = 0; i < header.patnum; i++)	// pattern offsets
    pattptr[i] = f->readInt(2);

  int adlibins = 0;
  for (i = 0; i < header.insnum; i++) {	// load instruments
    f->seek(insptr[i] * 16);
    if (f->error()) {
 	fp.close(f);
	return false;
    }
    inst[i].type = f->readInt(1);
    f->readString(inst[i].filename, 15);
    inst[i].d00 = f->readInt(1); inst[i].d01 = f->readInt(1);
    inst[i].d02 = f->readInt(1); inst[i].d03 = f->readInt(1);
    inst[i].d04 = f->readInt(1); inst[i].d05 = f->readInt(1);
    inst[i].d06 = f->readInt(1); inst[i].d07 = f->readInt(1);
    inst[i].d08 = f->readInt(1); inst[i].d09 = f->readInt(1);
    inst[i].d0a = f->readInt(1); inst[i].d0b = f->readInt(1);
    inst[i].volume = f->readInt(1);
    inst[i].dsk = f->readInt(1);
    f->ignore(2);
    inst[i].c2spd = f->readInt(4);
    f->ignore(12);
    f->readString(inst[i].name, 28);
    f->readString(inst[i].scri, 4);
    if (inst[i].type >= 2) {
      adlibins++;
      if (memcmp(inst[i].scri, "SCRI", 4)) {
	fp.close(f);
	return false;
      }
    }
  }
  if (!adlibins) { // no adlib instrument found
    fp.close(f);
    return false;
  }

  for (i = 0; i < header.patnum; i++) {	// load patterns
    f->seek(pattptr[i] * 16);
    if (f->error()) {
 	fp.close(f);
	return false;
    }
    load_pattern(i, f, f->readInt(2));
  }

  fp.close(f);
  rewind(0);
  return true;		// done
}

size_t Cs3mPlayer::load_pattern(int pat, binistream *f, size_t length) {
  struct {	// closure to keep track of amount read
    binistream *f;
    size_t length, count;
    unsigned char read() { return count++ < length ? f->readInt(1) : 0; }
  } fs = {f, length, 0};

  // read and unpack pattern data
  for (int row = 0; row < 64 && fs.count < length; row++) {
    while (unsigned char token = fs.read()) {
      s3mevent &ev = pattern[pat][row][token & 0x1f];

      if (token & 0x20) {	// note + instrument?
	unsigned char val = fs.read();
	ev.note = val & 0x0f;
	ev.oct = val >> 4;
	ev.instrument = fs.read();
      }

      if (token & 0x40) 	// volume?
	ev.volume = fs.read();

      if (token & 0x80) {	// command?
	ev.command = fs.read();
	ev.info = fs.read();
      }
    }
  }
  return fs.count;
}

bool Cs3mPlayer::update()
{
  // effect handling (timer dependent)
  for (int realchan = 0; realchan < 9; realchan++) {
    s3mchan &c = channel[realchan];	// shortcut ref
    const unsigned char info = c.info;	// fill infobyte cache

    switch (c.fx) {
    case 4:	// volume slide
    volslide:
      if (info <= 0x0f)				// volume slide down
	c.vol = std::max(c.vol - info, 0);
      if (!(info & 0x0f))			// volume slide up
	c.vol = std::min((c.vol + info) >> 4, 63);
      setvolume(realchan);
      break;

    case 5:	// slide down
      if (info == 0xf0 || info <= 0xe0) {
	slide_down(realchan, info);
	setfreq(realchan);
      }
      break;

    case 6:	// slide up
      if (info == 0xf0 || info <= 0xe0) {
	slide_up(realchan, info);
	setfreq(realchan);
      }
      break;

    case 7:	// tone portamento
      tone_portamento(realchan, c.dualinfo);
      break;

    case 8:	// vibrato
      vibrato(realchan, c.dualinfo);
      break;

    case 10:	// arpeggio
      c.nextfreq = c.freq;
      c.nextoct = c.oct;
      switch (c.trigger) {
      case 0:
	c.freq = notetable[c.note];
	break;
      case 1:
	c.freq = notetable[(c.note + (info >> 4)) % 12];
	c.oct += (c.note + (info >> 4)) / 12;
 	break;
      case 2:
	c.freq = notetable[(c.note + (info & 0x0f)) % 12];
	c.oct += (c.note + (info & 0x0f)) / 12;
	break;
      }
      if (++c.trigger > 2) c.trigger = 0;
      setfreq(realchan);
      c.freq = c.nextfreq;
      c.oct = c.nextoct;
      break;

    case 11:	// dual command: H00 and Dxy
      vibrato(realchan, c.dualinfo);
      goto volslide;

    case 12:	// dual command: G00 and Dxy
      tone_portamento(realchan, c.dualinfo);
      goto volslide;

    case 21:	// fine vibrato
      vibrato(realchan, info / 4);
      break;
    }
  }

  if (del) {		// speed compensation
    del--;
    return !songend;
  }

  // arrangement handling
  unsigned char pattnr;
  for (int end = 0;;) {
    pattnr = ord < header.ordnum ? orders[ord] : 0xff;
    if (pattnr < header.patnum) break;	// pattern is valid

    switch (pattnr) {
    default:	// skip invalid pattern
      AdPlug_LogWrite("Invalid pattern %d number (order %d)\n", pattnr, ord);
      // fallthrough;
    case 0xfe:	// "++" skip marker
      if (ord + 1 < header.ordnum) {
	ord++;
	break;
      }
      // else fallthrough;
    case 0xff:	// "--" end of song
      ord = 0;
      songend = 1;
      if (end++) return !songend;	// no order is a valid pattern
    }
  }

  // play row
  bool pattbreak = false;
  const unsigned char row = crow;	// fill row cache
  for (int chan = 0; chan < 32; chan++) {
    // resolve S3M -> AdLib channels
    const int realchan = header.chanset[chan] & 0x80 ? -1 : // channel disabled?
      chnresolv[header.chanset[chan] & 0x1f];
    if (realchan < 0) continue;		// channel playable?

    s3mchan &c = channel[realchan];
    const s3mevent &ev = pattern[pattnr][row][chan];

    // set channel values
    bool donote = false;

    // note
    if (ev.note < 12) {
      if (ev.command == 7 || ev.command == 12) {	// tone portamento
	c.nextfreq = notetable[ev.note];
	c.nextoct = ev.oct;
      } else {						// normal note
	c.note = ev.note;
	c.freq = notetable[ev.note];
	c.oct = ev.oct;
	c.key = 1;
	donote = true;
      }
    }

    // key off (is 14 here, cause note is only first 4 bits)
    if (ev.note == 14) {
      c.key = 0;
      setfreq(realchan);
    }

    // vibrato begins
    if ((c.fx != 8 && c.fx != 11) && (ev.command == 8 || ev.command == 11)) {
      c.nextfreq = c.freq;
      c.nextoct = c.oct;
    }
    // vibrato ends
    if ((ev.note >= 14) &&
	(c.fx == 8 || c.fx == 11) && (ev.command != 8 && ev.command != 11)) {
      c.freq = c.nextfreq;
      c.oct = c.nextoct;
      setfreq(realchan);
    }

    // set instrument
    if (ev.instrument > 0 && ev.instrument <= header.insnum) {
      c.inst = ev.instrument - 1;
      c.vol = std::min(inst[c.inst].volume, (unsigned char)63);
      if (ev.command != 7)
	donote = true;
    }

    // set command & infobyte
    c.fx = ev.command;
    if (ev.info)
      c.info = ev.info;
    // some commands reset the infobyte memory
    switch (c.fx) {
      case 1:
      case 2:
      case 3:
      case 20: c.info = ev.info;
    }

    // play note
    if (donote)
	playnote(realchan);

    // set volume
    if (ev.volume != 0xff) {
      c.vol = std::min(ev.volume, (unsigned char)63);
      setvolume(realchan);
    }

    // fill infobyte cache
    const unsigned char info = c.info, infoL = info & 0x0f, infoH = info >> 4;
    // command handling (row dependent)
    switch (c.fx) {
    case 1:	// set speed
      speed = info;
      break;

    case 2:	// jump to order
      if (info <= ord) songend = 1;
      ord = info;
      crow = 0;
      pattbreak = true;
      break;

    case 3:	// pattern break
      if (!pattbreak) {
	crow = info;
	if (!++ord) songend = 1;
	pattbreak = 1;
      }
      break;

    case 4:
      if (info > 0xf0)					// fine volume down
	c.vol = std::max(c.vol - infoL, 0);
      if (infoL == 0x0f && infoH)			// fine volume up
	c.vol = std::min(c.vol + infoH, 63);
      setvolume(realchan);
      break;

    case 5:
      if (info > 0xf0) {				// fine slide down
	slide_down(realchan, infoL);
	setfreq(realchan);
      }
      if (info > 0xe0 && info < 0xf0) {		// extra fine slide down
	slide_down(realchan, infoL / 4);
	setfreq(realchan);
      }
      break;

    case 6:
      if (info > 0xf0) {				// fine slide up
	slide_up(realchan, infoL);
	setfreq(realchan);
      }
      if(info > 0xe0 && info < 0xf0) {		// extra fine slide up
	slide_up(realchan, infoL / 4);
	setfreq(realchan);
      }
      break;

    case 7:	// tone portamento
    case 8:	// vibrato (remember info for dual commands)
      if (ev.info) c.dualinfo = info;
      break;

    case 10:	// arpeggio (set trigger)
      c.trigger = 0;
      break;

    case 19:
      if (info == 0xb0) {				// set loop start
	loopstart = row;
      } else if (infoH == 0xb) {			// pattern loop
	if (!loopcnt) {
	  loopcnt = infoL;
	  crow = loopstart;
	  pattbreak = true;
	} else if(--loopcnt > 0) {
	  crow = loopstart;
	  pattbreak = true;
	}
      }
      if (infoH == 0xe)					// patterndelay
	del = speed * infoL - 1;
      break;

    case 20:	// set tempo
      tempo = info;
      break;
    }
  }

  if (!del)
    del = speed - 1;		// speed compensation
  if (!pattbreak) {		// next row (only if no manual advance)
    if (++crow > 63) {
      crow = 0;
      if (!++ord) songend = 1;
      loopstart = 0;
    }
  }

  return !songend;		// still playing
}

void Cs3mPlayer::rewind(int subsong)
{
  // set basic variables
  songend = 0;
  ord = 0;
  crow = 0;
  tempo = header.it;
  speed = header.is;
  del = 0;
  loopstart = 0;
  loopcnt = 0;

  memset(channel, 0, sizeof(channel));

  opl->init();				// reset OPL chip
  opl->write(1, 32);			// Go to ym3812 mode
}

std::string Cs3mPlayer::gettype()
{
  std::string s("Scream Tracker ");

  switch (header.cwtv) {		// determine version number
  case 0x1300: return s + "3.00";
  case 0x1301: return s + "3.01";
  case 0x1303: return s + "3.03";
  case 0x1320: return s + "3.20";
  }
  return s + "3.??";
}

float Cs3mPlayer::getrefresh()
{
  return tempo / 2.5f;
}

/*** private methods *************************************/

void Cs3mPlayer::setvolume(unsigned char chan)
{
  const s3minst &instr = inst[channel[chan].inst];
  unsigned char op = op_table[chan], vol = channel[chan].vol;

  opl->write(0x43 + op,
	     (63*63 - (~instr.d03 & 63) * vol) / 63 + (instr.d03 & 192));
  if (instr.d0a & 1)
    opl->write(0x40 + op,
	       (63*63 - (~instr.d02 & 63) * vol) / 63 + (instr.d02 & 192));
}

void Cs3mPlayer::setfreq(unsigned char chan)
{
  opl->write(0xa0 + chan, channel[chan].freq & 0xff);
  opl->write(0xb0 + chan, (channel[chan].freq >> 8 & 0x03) +
	                  (channel[chan].oct << 2 & 0x1c) +
	                  (channel[chan].key ? 0x20 : 0));
}

void Cs3mPlayer::playnote(unsigned char chan)
{
  const s3minst &instr = inst[channel[chan].inst];
  unsigned char op = op_table[chan];

  opl->write(0xb0 + chan, 0);	// stop old note

  // set instrument data
  opl->write(0x20 + op, instr.d00);
  opl->write(0x23 + op, instr.d01);
  opl->write(0x40 + op, instr.d02);
  opl->write(0x43 + op, instr.d03);
  opl->write(0x60 + op, instr.d04);
  opl->write(0x63 + op, instr.d05);
  opl->write(0x80 + op, instr.d06);
  opl->write(0x83 + op, instr.d07);
  opl->write(0xe0 + op, instr.d08);
  opl->write(0xe3 + op, instr.d09);
  opl->write(0xc0 + chan, instr.d0a);

  // set frequency & play
  channel[chan].key = 1;
  setfreq(chan);
}

void Cs3mPlayer::slide_down(unsigned char chan, unsigned char amount)
{
  s3mchan &c = channel[chan];

  if (c.freq > 340 + amount) {
    c.freq -= amount;
  } else if (c.oct > 0) {
    c.oct--;
    c.freq = 684;
  } else {
    c.freq = 340;
  }
}

void Cs3mPlayer::slide_up(unsigned char chan, unsigned char amount)
{
  s3mchan &c = channel[chan];

  if (c.freq + amount < 686) {
    c.freq += amount;
  } else if (c.oct < 7) {
    c.oct++;
    c.freq = 341;
  } else {
    c.freq = 686;
  }
}

void Cs3mPlayer::vibrato(unsigned char chan, unsigned char info)
{
  unsigned char &trigger = channel[chan].trigger;

  unsigned char speed = info >> 4;
  unsigned char depth = 16 - (info & 0x0f) / 2;

  for (unsigned char i = 0; i < speed; i++) {
    trigger = (trigger + 1) % 64;

    if (trigger >= 16 && trigger < 48)
      slide_down(chan, vibratotab[trigger - 16] / depth);
    else if (trigger < 16)
      slide_up(chan, vibratotab[trigger + 16] / depth);
    else // trigger >= 48
      slide_up(chan, vibratotab[trigger - 48] / depth);
  }
  setfreq(chan);
}

void Cs3mPlayer::tone_portamento(unsigned char chan, unsigned char info)
{
  const s3mchan &c = channel[chan];

  if (c.freq + (c.oct << 10) < c.nextfreq + (c.nextoct << 10))
    slide_up(chan,info);
  if (c.freq + (c.oct << 10) > c.nextfreq + (c.nextoct << 10))
    slide_down(chan,info);

  setfreq(chan);
}
