/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2003 Simon Peter, <dn.tlp@gmx.net>, et al.
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
 * [xad] PSI player, by Riven the Mage <riven@ok.ru>
 */

/*
    - discovery -

  file(s) : 4BIDDEN.COM, PGRID.EXE
     type : Forbidden Dreams BBStro
            Power Grid BBStro
     tune : by Friar Tuck [Shadow Faction/ICE]
   player : by Psi [Future Crew]
  comment : seems to me what 4bidden tune & player was ripped from pgrid

  file(s) : MYSTRUNE.COM
     type : Mystical Runes BBStro
     tune : by ?
   player : by Psi [Future Crew]
*/

#include "psi.h"
#include "debug.h"

static unsigned short le16(const unsigned char *p)
{
  return p[0] | (p[1] << 8);
}

CPlayer *CxadpsiPlayer::factory(Copl *newopl)
{
  return new CxadpsiPlayer(newopl);
}

bool CxadpsiPlayer::xadplayer_load()
{
  if (xad.fmt != PSI || tune_size < 4)
    return false;

  // get header
  header.instr_ptr = le16(&tune[0]);
  header.seq_ptr = le16(&tune[2]);

  if (header.instr_ptr + (unsigned)psi.nchannels * 2 >= tune_size ||
      header.seq_ptr + (unsigned)psi.nchannels * 4 >= tune_size)
    return false;

  // calculate instruments & sequence tables
  psi.instr_table = &tune[header.instr_ptr];
  psi.seq_table = &tune[header.seq_ptr];

  // validate instrument data & sequence pointers
  for (int i = 0; i < psi.nchannels * 2; i += 2)
    if ((unsigned int)le16(&psi.instr_table[i]) + 11 >= tune_size)
      return false;
  for (int i = 0; i < psi.nchannels * 4; i += 2)
    if (le16(&psi.seq_table[i]) >= tune_size)
      return false;

  return true;
}

void CxadpsiPlayer::xadplayer_rewind(int subsong)
{
  static const unsigned char adlib_registers[][11] = { // last row unused
    { 0x20, 0x23, 0x40, 0x43, 0x60, 0x63, 0x80, 0x83, 0xE0, 0xE3, 0xC0 },
    { 0x21, 0x24, 0x41, 0x44, 0x61, 0x64, 0x81, 0x84, 0xE1, 0xE4, 0xC1 },
    { 0x22, 0x25, 0x42, 0x45, 0x62, 0x65, 0x82, 0x85, 0xE2, 0xE5, 0xC2 },
    { 0x28, 0x2B, 0x48, 0x4B, 0x68, 0x6B, 0x88, 0x8B, 0xE8, 0xEB, 0xC3 },
    { 0x29, 0x2C, 0x49, 0x4C, 0x69, 0x6C, 0x89, 0x8C, 0xE9, 0xEC, 0xC4 },
    { 0x2A, 0x2D, 0x4A, 0x4D, 0x6A, 0x6D, 0x8A, 0x8D, 0xEA, 0xED, 0xC5 },
    { 0x30, 0x33, 0x50, 0x53, 0x70, 0x73, 0x90, 0x93, 0xF0, 0xF3, 0xC6 },
    { 0x31, 0x34, 0x51, 0x54, 0x71, 0x74, 0x91, 0x94, 0xF1, 0xF4, 0xC7 },
    { 0x32, 0x35, 0x52, 0x55, 0x72, 0x75, 0x92, 0x95, 0xF2, 0xF5, 0xC8 },
  };

  opl_write(0x01, 0x20);
  opl_write(0x08, 0x00);
  opl_write(0xBD, 0x00);

  for (int i = 0; i < psi.nchannels; i++) {
    // define instruments
    unsigned short inspos = le16(&psi.instr_table[i * 2]);
    for (int j = 0; j < 11; j++)
      opl_write(adlib_registers[i][j], tune[inspos + j]);

    opl_write(0xA0 + i, 0x00);
    opl_write(0xB0 + i, 0x00);

    // reset sequence pointers
    psi.ptr[i] = le16(&psi.seq_table[i * 4]);

    psi.note_delay[i] = 1;
    psi.note_curdelay[i] = 1;
  }
  psi.looping = 0;
}

void CxadpsiPlayer::xadplayer_update()
{
  for (int i = 0; i < psi.nchannels; i++) {
    // time up for last event?
    if (--psi.note_curdelay[i]) continue;

    opl_write(0xA0 + i, 0x00);
    opl_write(0xB0 + i, 0x00);

    unsigned short &ptr = psi.ptr[i];
    unsigned char event = ptr < tune_size ? tune[ptr++] : 0;
#ifdef DEBUG
  AdPlug_LogWrite("channel %02X, event %02X:\n", i+1, event);
#endif

    // end of sequence?
    if (!event) {
      ptr = le16(&psi.seq_table[i * 4 + 2]);
      event = tune[ptr++]; // loop position is validated
#ifdef DEBUG
  AdPlug_LogWrite(" channel %02X, event %02X:\n", i+1, event);
#endif

      // set sequence loop flag
      psi.looping |= 1 << i;
      // module loop (all flags set)?
      plr.looping = psi.looping == (1 << psi.nchannels) - 1;
    }

    // new note delay?
    if (event & 0x80) {
      psi.note_delay[i] = event & 0x7F;

      event = ptr < tune_size ? tune[ptr++] : 0;
#ifdef DEBUG
  AdPlug_LogWrite("  channel %02X, event %02X:\n", i+1, event);
#endif
    }

    psi.note_curdelay[i] = psi.note_delay[i];

    // play note
    struct Note { unsigned char hi, lo; };
    static const Note notes[16] = {
      { 0x21, 0x6B }, { 0x21, 0x81 }, { 0x21, 0x98 }, { 0x21, 0xB0 },
      { 0x21, 0xCA }, { 0x21, 0xE5 }, { 0x22, 0x02 }, { 0x22, 0x20 },
      { 0x22, 0x41 }, { 0x22, 0x63 }, { 0x22, 0x87 }, { 0x23, 0x64 },
      // rest is zero
    };
    Note note = notes[event & 0x0F];

    opl_write(0xA0 + i, note.lo);
    opl_write(0xB0 + i, note.hi + ((event & 0xF0) >> 2));
  }
}

float CxadpsiPlayer::xadplayer_getrefresh()
{
  return 70.0f;
}

std::string CxadpsiPlayer::xadplayer_gettype()
{
  return std::string("xad: psi player");
}

unsigned int CxadpsiPlayer::xadplayer_getinstruments()
{
  return psi.nchannels;
}
