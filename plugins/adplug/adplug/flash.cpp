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
 * [xad] FLASH player, by Riven the Mage <riven@ok.ru>
 */

/*
    - discovery -

  file(s) : LA-INTRO.EXE
     type : Lunatic Asylum BBStro
     tune : by Rogue [Logic Design]
   player : by Flash [Logic Design]
*/

#include "flash.h"
#include "debug.h"

static const unsigned char flash_adlib_registers[9][11] = {
  { 0x23, 0x20, 0x43, 0x40, 0x63, 0x60, 0x83, 0x80, 0xC0, 0xE3, 0xE0 },
  { 0x24, 0x21, 0x44, 0x41, 0x64, 0x61, 0x84, 0x81, 0xC1, 0xE4, 0xE1 },
  { 0x25, 0x22, 0x45, 0x42, 0x65, 0x62, 0x85, 0x82, 0xC2, 0xE5, 0xE2 },
  { 0x2B, 0x28, 0x4B, 0x48, 0x6B, 0x68, 0x8B, 0x88, 0xC3, 0xEB, 0xE8 },
  { 0x2C, 0x29, 0x4C, 0x49, 0x6C, 0x69, 0x8C, 0x89, 0xC4, 0xEC, 0xE9 },
  { 0x2D, 0x2A, 0x4D, 0x4A, 0x6D, 0x6A, 0x8D, 0x8A, 0xC5, 0xED, 0xEA },
  { 0x33, 0x30, 0x53, 0x50, 0x73, 0x70, 0x93, 0x90, 0xC6, 0xF3, 0xF0 },
  { 0x34, 0x31, 0x54, 0x51, 0x74, 0x71, 0x94, 0x91, 0xC7, 0xF4, 0xF1 },
  { 0x35, 0x32, 0x55, 0x52, 0x75, 0x72, 0x95, 0x92, 0xC8, 0xF5, 0xF2 }
};

static const unsigned short flash_notes[12] = {
  0x157, 0x16B, 0x181, 0x198, 0x1B0, 0x1CA,
  0x1E5, 0x202, 0x220, 0x241, 0x263, 0x287
};

// unused:
static const unsigned char flash_default_instrument[8] = {
  0x00, 0x00, 0x3F, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF
};

CPlayer *CxadflashPlayer::factory(Copl *newopl)
{
  return new CxadflashPlayer(newopl);
}

void CxadflashPlayer::xadplayer_rewind(int subsong)
{
  plr.speed = xad.speed;

  flash.order_pos = 0;
  flash.pattern_pos = 0;

  opl_write(0x08, 0x00);
  opl_write(0xBD, 0x00);

  // clear channels
  for (int i = 0; i < 9; i++) {
    opl_write(0xA0 + i, 0x00);
    opl_write(0xB0 + i, 0x00);
  }

  // assign instruments
  for (int i = 0; i < 9; i++)
    for (int j = 0; j < 11; j++)
      opl_write(flash_adlib_registers[i][j], tune[i*12 + j]);
}

void CxadflashPlayer::xadplayer_update()
{
  const unsigned char *order = &tune[0x600];
  unsigned short event_pos = 0x633 + order[flash.order_pos] * 1152
                           + flash.pattern_pos * 18;

  for (int i = 0; i < 9; i++) {
    if (event_pos > tune_size - 2) {
      flash.pattern_pos = 0x3F; // end of data, skip rest of pattern
      break;
    }

    unsigned char event_b0 = tune[event_pos++];
    unsigned char event_b1 = tune[event_pos++];
#ifdef DEBUG
   AdPlug_LogWrite("channel %02X, event %02X %02X:\n", i+1, event_b0, event_b1);
#endif

    if (event_b0 == 0x80) {             // 0.0x80: Set Instrument
      if (event_b1 < 0x80)
        for (int j = 0; j < 11; j++)
          opl_write(flash_adlib_registers[i][j], tune[event_b1 * 12 + j]);
      continue;
    }

    signed char slide_freq = 0;
    unsigned char fx = (event_b1 >> 4), fx_p = (event_b1 & 0x0F);

    switch(fx) {
    case 0x0:
      if (event_b1 == 0x01)             // 1.0x01: Pattern Break
        flash.pattern_pos = 0x3F;
      break;

    case 0x1:                           // 1.0x1y: Fine Frequency Slide Up
      slide_freq = fx_p << 1;
      break;

    case 0x2:                           // 1.0x2y: Fine Frequency Slide Down
      slide_freq = -(fx_p << 1);
      break;

    case 0xA:                           // 1.0xAy: Set Carrier volume
      opl_write(flash_adlib_registers[i][2], fx_p << 2);
      break;

    case 0xB:                           // 1.0xBy: Set Modulator volume
      opl_write(flash_adlib_registers[i][3], fx_p << 2);
      break;

    case 0xC:                           // 1.0xCy: Set both operators volume
      opl_write(flash_adlib_registers[i][2], fx_p << 2);
      opl_write(flash_adlib_registers[i][3], fx_p << 2);
      break;

//  case 0xE:                           // 1.0xEy: ? (increase some value)

    case 0xF:                           // 1.0xFy: Set Speed
      plr.speed = (fx_p + 1);
      break;
    }

    if (event_b0) {
      // mute channel
      opl_write(0xA0 + i, adlib[0xA0 + i]);
      opl_write(0xB0 + i, adlib[0xB0 + i] & 0xDF);

      // is note?
      if (event_b0 != 0x7F) {
        unsigned char note = (event_b0 - 1) % 12;
        unsigned char octave = (event_b0 - 1) / 12;
        unsigned short freq = flash_notes[note] | (octave << 10) | 0x2000;

        opl_write(0xA0 + i, freq & 0xFF);
        opl_write(0xB0 + i, freq >> 8);
      }
    }

    if (slide_freq) {
      unsigned short freq = (adlib[0xB0 + i] << 8) + adlib[0xA0 + i];
      freq += slide_freq;
	
      opl_write(0xA0 + i, freq & 0xFF);
      opl_write(0xB0 + i, freq >> 8);
    }
  }

  // next row
  flash.pattern_pos++;

  // end of pattern ?
  if (flash.pattern_pos >= 0x40)
  {
    flash.pattern_pos = 0;
    flash.order_pos++;

    // end of module ?
    if (flash.order_pos > 0x33 || order[flash.order_pos] == 0xFF)
    {
      flash.order_pos = 0;
      plr.looping = 1;
    }
  }
}

float CxadflashPlayer::xadplayer_getrefresh()
{
  return 17.5f;
}

std::string CxadflashPlayer::xadplayer_gettype()
{
  return std::string("xad: flash player");
}

unsigned int CxadflashPlayer::xadplayer_getinstruments()
{
  return 128;
}
