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
 * Johannes Bjerregaard's JBM Adlib Music Format player for AdPlug
 * Written by Dennis Lindroos <lindroos@nls.fi>, February-March 2007
 * - Designed and coded from scratch (only frequency-table taken from MUSIC.BIN)
 * - The percussion mode is buggy (?) but i'm not good enough to find them
 *   and honestly i think the melodic-mode tunes are much better ;)
 *
 * This version doesn't use the binstr.h functions (coded with custom func.)
 * This is my first attempt on writing a musicplayer for AdPlug, and i'm not
 * coding C++ very often.. 
 *
 * Released under the terms of the GNU General Public License.
 */

#include "jbm.h"

static const unsigned short notetable[96] = {
  0x0158, 0x016d, 0x0183, 0x019a, 0x01b2, 0x01cc, 0x01e7, 0x0204,
  0x0223, 0x0244, 0x0266, 0x028b, 0x0558, 0x056d, 0x0583, 0x059a,
  0x05b2, 0x05cc, 0x05e7, 0x0604, 0x0623, 0x0644, 0x0666, 0x068b,
  0x0958, 0x096d, 0x0983, 0x099a, 0x09b2, 0x09cc, 0x09e7, 0x0a04,
  0x0a23, 0x0a44, 0x0a66, 0x0a8b, 0x0d58, 0x0d6d, 0x0d83, 0x0d9a,
  0x0db2, 0x0dcc, 0x0de7, 0x0e04, 0x0e23, 0x0e44, 0x0e66, 0x0e8b,
  0x1158, 0x116d, 0x1183, 0x119a, 0x11b2, 0x11cc, 0x11e7, 0x1204,
  0x1223, 0x1244, 0x1266, 0x128b, 0x1558, 0x156d, 0x1583, 0x159a,
  0x15b2, 0x15cc, 0x15e7, 0x1604, 0x1623, 0x1644, 0x1666, 0x168b,
  0x1958, 0x196d, 0x1983, 0x199a, 0x19b2, 0x19cc, 0x19e7, 0x1a04,
  0x1a23, 0x1a44, 0x1a66, 0x1a8b, 0x1d58, 0x1d6d, 0x1d83, 0x1d9a,
  0x1db2, 0x1dcc, 0x1de7, 0x1e04, 0x1e23, 0x1e44, 0x1e66, 0x1e8b
};

static const unsigned char percmx_tab[4] = { 0x14, 0x12, 0x15, 0x11 };
static const unsigned char perchn_tab[5] = { 6, 7, 8, 8, 7 };
static unsigned char percmaskoff[5] = { 0xef, 0xf7, 0xfb, 0xfd, 0xfe };
static unsigned char percmaskon[5] =  { 0x10, 0x08, 0x04, 0x02, 0x01 };

static inline unsigned short GET_WORD(unsigned char *b, int x)
{
  return ((unsigned short)(b[x+1] << 8) | b[x]);
}

/*** public methods *************************************/

CPlayer *CjbmPlayer::factory(Copl *newopl)
{
  return new CjbmPlayer(newopl);
}

bool CjbmPlayer::load(const std::string &filename, const CFileProvider &fp)
{
  binistream    *f = fp.open(filename); if(!f) return false;
  unsigned int   filelen = fp.filesize(f);
  int            i;

  if (!filelen || !fp.extension(filename, ".jbm")) goto loaderr;

  // Allocate memory buffer m[] and read entire file into it

  m = new unsigned char[filelen];
  if (f->readString((char *)m, filelen) != filelen) goto loaderr;

  fp.close(f);

  // The known .jbm files always seem to start with the number 0x0002

  if (GET_WORD(m, 0) != 0x0002)
    return false;

  // Song tempo

  i = GET_WORD(m, 2);
  timer = 1193810.0 / (i ? i : 0xffff);

  seqtable = GET_WORD(m, 4);
  instable = GET_WORD(m, 6);

  // The flags word has atleast 1 bit, the Adlib's rhythm mode, but
  // currently we don't support that :(

  flags = GET_WORD(m, 8);

  // Instrument datas are directly addressed with m[] 

  inscount = (filelen - instable) >> 4;

  // Voice' and sequence pointers

  seqcount = 0xffff;
  for (i = 0; i < 11; i++) {
    voice[i].trkpos = voice[i].trkstart = GET_WORD(m, 10 + (i<<1));
    if (voice[i].trkpos && voice[i].trkpos < seqcount)
      seqcount = voice[i].trkpos;
  }
  seqcount = (seqcount - seqtable) >> 1;
  sequences = new unsigned short[seqcount];
  for (i = 0; i < seqcount; i++) 
    sequences[i] = GET_WORD(m, seqtable + (i<<1));

  rewind(0);
  return true;
 loaderr:
  fp.close(f);
  return false;
}

bool CjbmPlayer::update()
{
  short c, spos, frq;

  for (c = 0; c < 11; c++) {
    if (!voice[c].trkpos)		// Unused channel
      continue;

    if (--voice[c].delay)
      continue;

    // Turn current note/percussion off

    if (voice[c].note&0x7f)
      opl_noteonoff(c, &voice[c], 0);

    // Process events until we have a note

    spos = voice[c].seqpos;
    while(!voice[c].delay) {
      switch(m[spos]) {
      case 0xFD:	// Set Instrument
	voice[c].instr = m[spos+1];
	set_opl_instrument(c, &voice[c]);
	spos+=2;
	break;
      case 0xFF:	// End of Sequence
	voice[c].seqno = m[++voice[c].trkpos];
	if (voice[c].seqno == 0xff) {
	  voice[c].trkpos = voice[c].trkstart;
	  voice[c].seqno = m[voice[c].trkpos];
	  //voicemask &= 0x7ff-(1<<c);
	  voicemask &= ~(1<<c);
	}
	spos = voice[c].seqpos = sequences[voice[c].seqno];
	break;
      default:	// Note Event
	if ((m[spos] & 127) > 95)
	  return 0;

	voice[c].note = m[spos];
	voice[c].vol = m[spos+1];
	voice[c].delay =
	  (m[spos+2] + (m[spos+3]<<8)) + 1;

	frq = notetable[voice[c].note&127];
	voice[c].frq[0] = (unsigned char)frq;
	voice[c].frq[1] = frq >> 8;
	spos+=4;
      }
    }
    voice[c].seqpos = spos;

    // Write new volume to the carrier operator, or percussion

    if (flags&1 && c > 6)
      opl->write(0x40 + percmx_tab[c-7], voice[c].vol ^ 0x3f);
    else if (c < 9)
      opl->write(0x43 + op_table[c], voice[c].vol ^ 0x3f);

    // Write new frequencies and Gate bit

    opl_noteonoff(c, &voice[c], !(voice[c].note & 0x80));
  }
  return (!!voicemask);
}

void CjbmPlayer::rewind(int subsong)
{
  int c;

  voicemask = 0;

  for (c = 0; c < 11; c++) {
    voice[c].trkpos = voice[c].trkstart;

    if (!voice[c].trkpos) continue;

    voicemask |= (1<<c);

    voice[c].seqno = m[voice[c].trkpos];
    voice[c].seqpos = sequences[voice[c].seqno];

    voice[c].note = 0;
    voice[c].delay = 1;
  }

  opl->init();
  opl->write(0x01, 32);

  // Set rhythm mode if flags bit #0 is set
  // AM and Vibrato are full depths (taken from DosBox RAW output)
  bdreg = 0xC0 | (flags&1)<<5;

  opl->write(0xbd, bdreg);

#if 0
  if (flags&1) {
    voice[7].frq[0] = 0x58; voice[7].frq[1] = 0x09; // XXX
    voice[8].frq[0] = 0x04; voice[8].frq[1] = 0x0a; // XXX
    opl_noteonoff(7, &voice[7], 0);
    opl_noteonoff(8, &voice[8], 0);
  }
#endif

  return;
}

/*** private methods ************************************/

void CjbmPlayer::opl_noteonoff(int channel, JBMVoice *v, bool state)
{
  if (flags&1 && channel > 5) {
    // Percussion
    opl->write(0xa0 + perchn_tab[channel-6], voice[channel].frq[0]);
    opl->write(0xb0 + perchn_tab[channel-6], voice[channel].frq[1]);
    opl->write(0xbd,
	       state ? bdreg | percmaskon[channel-6] :
	       bdreg & percmaskoff[channel-6]);
  } else {
    // Melodic mode or Rhythm mode melodic channels
    opl->write(0xa0 + channel, voice[channel].frq[0]);
    opl->write(0xb0 + channel,
	       state ? voice[channel].frq[1] | 0x20 :
	       voice[channel].frq[1] & 0x1f);
  }
  return;
}


void CjbmPlayer::set_opl_instrument(int channel, JBMVoice *v)
{
  short i = instable + (v->instr << 4);

  // Sanity check on instr number - or we'll be reading outside m[] !

  if (v->instr >= inscount)
    return;

  // For rhythm mode, multiplexed drums. I don't care about waveforms!
  if ((flags&1) & (channel > 6)) {
    opl->write(0x20 + percmx_tab[channel-7], m[i+0]);
    opl->write(0x40 + percmx_tab[channel-7], m[i+1] ^ 0x3f);
    opl->write(0x60 + percmx_tab[channel-7], m[i+2]);
    opl->write(0x80 + percmx_tab[channel-7], m[i+3]);

    opl->write(0xc0 + perchn_tab[channel-6], m[i+8]&15);
    return;
  }

  if (channel >= 9)
    return;

  // AM/VIB/EG/KSR/FRQMUL, KSL/OUTPUT, ADSR for 1st operator
  opl->write(0x20 + op_table[channel], m[i+0]);
  opl->write(0x40 + op_table[channel], m[i+1] ^ 0x3f);
  opl->write(0x60 + op_table[channel], m[i+2]);
  opl->write(0x80 + op_table[channel], m[i+3]);

  // AM/VIB/EG/KSR/FRQMUL, KSL/OUTPUT, ADSR for 2nd operator
  opl->write(0x23 + op_table[channel], m[i+4]);
  opl->write(0x43 + op_table[channel], m[i+5] ^ 0x3f);
  opl->write(0x63 + op_table[channel], m[i+6]);
  opl->write(0x83 + op_table[channel], m[i+7]);

  // WAVEFORM for operators
  opl->write(0xe0 + op_table[channel], (m[i+8]>>4)&3);
  opl->write(0xe3 + op_table[channel], (m[i+8]>>6)&3);

  // FEEDBACK/FM mode
  opl->write(0xc0 + channel, m[i+8]&15);
	
  return;
}
