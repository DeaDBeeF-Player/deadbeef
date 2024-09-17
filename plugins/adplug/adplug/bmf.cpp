/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2003, 2006 Simon Peter, <dn.tlp@gmx.net>, et al.
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
 * [xad] BMF player, by Riven the Mage <riven@ok.ru>
 */

/*
    - discovery -

  file(s) : GAMESNET.COM
     type : GamesNet advertising intro
     tune : by (?)The Brain [Razor 1911]
   player : ver.0.9b by Hammer

  file(s) : 2FAST4U.COM
     type : Ford Knox BBStro
     tune : by The Brain [Razor 1911]
   player : ver.1.1 by ?
  comment : in original player at 9th channel the feedback adlib register is
            not C8 but C6.

  file(s) : DATURA.COM
     type : Datura BBStro
     tune : by The Brain [Razor 1911]
   player : ver.1.2 by ?
  comment : inaccurate replaying, because constant outport; in original player
            it can be 380 or 382.
*/

#include "strnlen.h"
#include "bmf.h"
#include "debug.h"

const unsigned char CxadbmfPlayer::bmf_adlib_registers[117] = {
  0x20, 0x23, 0x40, 0x43, 0x60, 0x63, 0x80, 0x83, 0xA0, 0xB0, 0xC0, 0xE0, 0xE3,
  0x21, 0x24, 0x41, 0x44, 0x61, 0x64, 0x81, 0x84, 0xA1, 0xB1, 0xC1, 0xE1, 0xE4,
  0x22, 0x25, 0x42, 0x45, 0x62, 0x65, 0x82, 0x85, 0xA2, 0xB2, 0xC2, 0xE2, 0xE5,
  0x28, 0x2B, 0x48, 0x4B, 0x68, 0x6B, 0x88, 0x8B, 0xA3, 0xB3, 0xC3, 0xE8, 0xEB,
  0x29, 0x2C, 0x49, 0x4C, 0x69, 0x6C, 0x89, 0x8C, 0xA4, 0xB4, 0xC4, 0xE9, 0xEC,
  0x2A, 0x2D, 0x4A, 0x4D, 0x6A, 0x6D, 0x8A, 0x8D, 0xA5, 0xB5, 0xC5, 0xEA, 0xED,
  0x30, 0x33, 0x50, 0x53, 0x70, 0x73, 0x90, 0x93, 0xA6, 0xB6, 0xC6, 0xF0, 0xF3,
  0x31, 0x34, 0x51, 0x54, 0x71, 0x74, 0x91, 0x94, 0xA7, 0xB7, 0xC7, 0xF1, 0xF4,
  0x32, 0x35, 0x52, 0x55, 0x72, 0x75, 0x92, 0x95, 0xA8, 0xB8, 0xC8, 0xF2, 0xF5
};

const unsigned short CxadbmfPlayer::bmf_notes[12] = {
  0x157, 0x16B, 0x181, 0x198, 0x1B0, 0x1CA,
  0x1E5, 0x202, 0x220, 0x241, 0x263, 0x287
};

/* for 1.1 */
const unsigned short CxadbmfPlayer::bmf_notes_2[12] = {
  0x159, 0x16D, 0x183, 0x19A, 0x1B2, 0x1CC,
  0x1E8, 0x205, 0x223, 0x244, 0x267, 0x28B
};

const unsigned char CxadbmfPlayer::bmf_default_instrument[13] = {
  0x01, 0x01, 0x3F, 0x3F, 0x00, 0x00, 0xF0, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00
};

CPlayer *CxadbmfPlayer::factory(Copl *newopl)
{
  return new CxadbmfPlayer(newopl);
}

bool CxadbmfPlayer::xadplayer_load()
{
  size_t ptr = 0;
  int i;

  if (xad.fmt != BMF)
    return false;

#ifdef DEBUG
  AdPlug_LogWrite("\nbmf_load():\n\n");
#endif
  if (tune_size < 6)
    return false;
  if (!memcmp(&tune[0], "BMF1.2", 6)) {
    bmf.version = BMF1_2;
    bmf.timer = 70.0f;
    ptr = 6;
  } else if (!memcmp(&tune[0], "BMF1.1", 6)) {
    bmf.version = BMF1_1;
    bmf.timer = 68.5f;
    ptr = 6;
  } else {
    bmf.version = BMF0_9B;
    bmf.timer = 18.2f;
  }

  // copy title & author
  if (bmf.version > BMF0_9B) {
    // It's unclear whether title/author can be longer than 35 chars. The
    // implementation reads an arbitrarily long NUL-terminated string and
    // truncates it to fit in a fixed 36 byte buffer. Doesn't make sense!
    size_t len = strnlen((char *)&tune[ptr], tune_size - ptr);
    if (ptr + len == tune_size)
      return false;
    if (len >= sizeof(bmf.title)) {
      memcpy(bmf.title, &tune[ptr], sizeof(bmf.title) - 1);
      bmf.title[sizeof(bmf.title) - 1] = 0;
    } else {
      memcpy(bmf.title, &tune[ptr], len + 1);
    }
    ptr += len + 1;

    len = strnlen((char *)&tune[ptr], tune_size - ptr);
    if (ptr + len == tune_size)
      return false;
    if (len >= sizeof(bmf.author)) {
      memcpy(bmf.author, &tune[ptr], sizeof(bmf.author) - 1);
      bmf.author[sizeof(bmf.author) - 1] = 0;
    } else {
      memcpy(bmf.author, &tune[ptr], len + 1);
    }
    ptr += len + 1;
  } else {
    // bmf.version == BMF0_9B
    strncpy(bmf.title, xad.title, sizeof(bmf.title));
    bmf.title[sizeof(bmf.title) - 1] = 0;
    strncpy(bmf.author, xad.author, sizeof(bmf.author));
    bmf.author[sizeof(bmf.author) - 1] = 0;
  }

  // speed
  if (tune_size - ptr < 1)
    return false;
  bmf.speed = tune[ptr++];
  if (bmf.version == BMF0_9B)
    bmf.speed /= 3; // removed pointless shifts from original code

  // load instruments
  if (bmf.version > BMF0_9B) {
    if (tune_size - ptr < 4)
      return false;
    unsigned long iflags = (((unsigned long)tune[ptr]  ) << 24) |
                           (((unsigned long)tune[ptr+1]) << 16) |
                           (((unsigned long)tune[ptr+2]) <<  8) |
                            ((unsigned long)tune[ptr+3]);
    ptr += 4;

    for (i = 0; i < 32; i++) {
      if (iflags & (1 << (31-i))) {
        if (tune_size - ptr < sizeof(bmf.instruments[i]))
          return false;
        memcpy(bmf.instruments[i].name, &tune[ptr],
                sizeof(bmf.instruments[i].name) - 1);
        // last char ignored since name must be 0-terminated
        bmf.instruments[i].name[sizeof(bmf.instruments[i].name) - 1] = 0;
        memcpy(bmf.instruments[i].data,
               &tune[ptr + sizeof(bmf.instruments[i].name)],
               sizeof(bmf.instruments[i].data));
        ptr += sizeof(bmf.instruments[i]);
      } else if (bmf.version == BMF1_1) {
        memset(bmf.instruments[i].name, 0, sizeof(bmf.instruments[i].name));
        memcpy(bmf.instruments[i].data, bmf_default_instrument,
               sizeof(bmf.instruments[i].data));
      } else {
        memset(&bmf.instruments[i], 0, sizeof(bmf.instruments[i]));
      }
    }
  } else {
    // bmf.version == BMF0_9B
    ptr = 6;

    if (tune_size - ptr < 32 * 15)
      return false;
    // Should unset entries be zero or set to bmf_default_instrument?
    memset(bmf.instruments, 0, sizeof(bmf.instruments));
    for (i = 0; i < 32; i++) {
      size_t ip = ptr + 15 * i;
      // Original code lacked check and had a comment: "bug no.1 (no
      // instrument-table-end detection)" - what does that mean?
      if (tune[ip] < 32)
        memcpy(bmf.instruments[tune[ip]].data, &tune[ip + 2],
               sizeof(bmf.instruments[tune[ip]].data));
      else // what? continue, return false, or skip rest of the table?
        break; // Old comment (see above) suggests it's a table end marker.
    }
    ptr += 32 * 15;
  }

  // load streams
  if (bmf.version > BMF0_9B) {
    if (tune_size - ptr < 4)
      return false;
    unsigned long sflags = (((unsigned long)tune[ptr  ]) << 24) |
                           (((unsigned long)tune[ptr+1]) << 16) |
                           (((unsigned long)tune[ptr+2]) <<  8) |
                            ((unsigned long)tune[ptr+3]);
    ptr += 4;

    for (i = 0; i < 9; i++)
      if (sflags & (1 << (31-i))) {
        long len = __bmf_convert_stream(tune + ptr, i, tune_size - ptr);
        if (len < 0)
          return false;
        ptr += len;
      } else {
        bmf.streams[i][0].cmd = 0xFF;
      }
  } else {
    // bmf.version == BMF0_9B
    if (tune[5] > 9)
      return false;
    for (i = 0; i < tune[5]; i++) {
      long len = __bmf_convert_stream(tune + ptr, i, tune_size - ptr);
      if (len < 0)
        return false;
      ptr += len;
    }

    for (i = tune[5]; i < 9; i++)
      bmf.streams[i][0].cmd = 0xFF;
  }

  return true;
}

void CxadbmfPlayer::xadplayer_rewind(int subsong)
{
  memset(bmf.channel, 0, sizeof(bmf.channel));

  plr.speed = bmf.speed;
#ifdef DEBUG
  AdPlug_LogWrite("speed: %x\n", plr.speed);
#endif

  bmf.active_streams = 9;

  // OPL initialization
  if (bmf.version > BMF0_9B) {
    opl_write(0x01, 0x20);

    /* 1.1 */
    int i, j;
    if (bmf.version == BMF1_1)
      for (i = 0; i < 9; i++)
        for (j = 0; j < 13; j++)
          opl_write(bmf_adlib_registers[13 * i + j], bmf_default_instrument[j]);
    /* 1.2 */
    else if (bmf.version == BMF1_2)
      for (i = 0x20; i < 0x100; i++)
        opl_write(i, 0xFF); // very interesting, really!
  }

  /* ALL */
  opl_write(0x08, 0x00);
  opl_write(0xBD, 0xC0);
}

void CxadbmfPlayer::xadplayer_update()
{
  for (int i = 0; i < 9; i++) {
    unsigned short &pos = bmf.channel[i].stream_position;
    if (pos == 0xFFFF)
      continue;

    if (bmf.channel[i].delay) {
      bmf.channel[i].delay--;
      continue;
    }

#ifdef DEBUG
    AdPlug_LogWrite("channel %02X:\n", i);
#endif

again:
    const bmf_event &event = bmf.streams[i][pos];
#ifdef DEBUG
    AdPlug_LogWrite("%02X %02X %02X %02X %02X %02X\n",
                    event.note, event.delay, event.volume,
                    event.instrument, event.cmd, event.cmd_data);
#endif

    switch (event.cmd) {
      // Process so-called Cross-Events

    case 0xFF: // End of Stream
      pos = 0xFFFF;
      bmf.active_streams--;
      continue;

    case 0xFE: // Save Loop Position
      bmf.channel[i].loop_position = ++pos;
      bmf.channel[i].loop_counter = event.cmd_data;
      goto again;

    case 0xFD: // Loop to Saved Position
      if (bmf.channel[i].loop_counter) {
        pos = bmf.channel[i].loop_position;
        bmf.channel[i].loop_counter--;
      } else {
        pos++;
      }
      goto again;

      // Process normal event

    case 0x01: // Set Modulator Volume
      {
        unsigned char reg = bmf_adlib_registers[13 * i + 2];
        // Masking cmd_data to avoid out of range writes might be a good idea,
        // or do it while parsing the file.
        opl_write(reg, (adlib[reg] | 0x3F) - event.cmd_data);
      }
      break;

    case 0x10: // Set Speed
      plr.speed = event.cmd_data;
      plr.speed_counter = plr.speed;
      break;
    }

    // Got normal event, pos != 0xFFFF

    // Process delay
    bmf.channel[i].delay = event.delay;

    // Process instrument
    if (event.instrument) {
      unsigned char ins = event.instrument - 1;

      if (bmf.version != BMF1_1)
        opl_write(0xB0 + i, adlib[0xB0 + i] & 0xDF);

      for (int j = 0; j < 13; j++)
        opl_write(bmf_adlib_registers[13 * i + j],
                  bmf.instruments[ins].data[j]);
    }

    // Process volume
    if (event.volume) {
      unsigned char vol = event.volume - 1;
      unsigned char reg = bmf_adlib_registers[13 * i + 3];

      // Masking cmd_data to avoid out of range writes might be a good idea,
      // or do it while parsing the file.
      opl_write(reg, (adlib[reg] | 0x3F) - vol);
    }

    // Process note
    if (event.note) {
      unsigned short note = event.note - 1;
      unsigned short freq = 0;

      // mute channel
      opl_write(0xB0 + i, adlib[0xB0 + i] & 0xDF);

      // get frequency
      if (bmf.version == BMF1_1) {
        if (note < 0x60)
          freq = bmf_notes_2[note % 12];
      } else {
        if (note != 0x7E) // really?
          freq = bmf_notes[note % 12];
      }

      // play note
      if (freq) {
        opl_write(0xB0 + i, (freq >> 8) | ((note / 12) << 2) | 0x20);
        opl_write(0xA0 + i, freq & 0xFF);
      }
    } // if (event.note)

    pos++;
  } // for (i)

  // is module loop ?
  if (!bmf.active_streams) {
    for (int j = 0; j < 9; j++)
      bmf.channel[j].stream_position = 0;

    bmf.active_streams = 9;
    plr.looping = 1;
  }
}

float CxadbmfPlayer::xadplayer_getrefresh()
{
  return bmf.timer;
}

std::string CxadbmfPlayer::xadplayer_gettype()
{
  return std::string("xad: BMF Adlib Tracker");
}

std::string CxadbmfPlayer::xadplayer_gettitle()
{
  return std::string(bmf.title);
}

std::string CxadbmfPlayer::xadplayer_getauthor()
{
  return std::string(bmf.author);
}

unsigned int CxadbmfPlayer::xadplayer_getinstruments()
{
  return 32;
}

std::string CxadbmfPlayer::xadplayer_getinstrument(unsigned int i)
{
  return std::string(bmf.instruments[i].name);
}

unsigned int CxadbmfPlayer::xadplayer_getspeed()
{
  return plr.speed;
}

/* -------- Internal Functions ---------------------------- */

long int CxadbmfPlayer::__bmf_convert_stream(const unsigned char *stream,
  int channel, unsigned long stream_size)
{
#ifdef DEBUG
  AdPlug_LogWrite("channel %02X (note, delay, volume, instrument, "
                  "command, command_data):\n", channel);
  const unsigned char *last = stream;
#endif
  const unsigned char *const stream_start = stream;
  const unsigned char *const stream_end = stream + stream_size;
  const int last_pos = sizeof(bmf.streams[0]) / sizeof(bmf.streams[0][0]) - 1;

  // The loop exits when End of Stream (0xFE) is found (or on error).
  // If the stream is too long to store, truncate it, but keep reading
  // until the end marker is found and stored.
  for (int pos = 0; pos <= last_pos; pos < last_pos && pos++) {
    bmf_event &event = bmf.streams[channel][pos];
    memset(&event, 0, sizeof(bmf_event));

    if (stream_end - stream < 1)
      return -1;
    switch (*stream) {
      /* Cross-Events */
    case 0xFE: // 0xFE -> 0xFF: End of Stream
      event.cmd = 0xFF;
      stream++;
      pos = last_pos + 1; // end loop
      break;

    case 0xFC: // 0xFC -> 0xFE xx: Save Loop Position
      event.cmd = 0xFE;
      if (stream_end - stream < 2)
        return -1;
      event.cmd_data = (stream[1] & (bmf.version == BMF0_9B ? 0x7F : 0x3F)) - 1;
      stream += 2;
      break;

    case 0x7D: // 0x7D -> 0xFD: Loop to Saved Position
      event.cmd = 0xFD;
      stream++;
      break;

      /* Normal Events */
    default:
      // The event is defined by the next 1 to 4 bytes and consists of
      // a note, an optional delay, and an optional command (which may
      // take another data byte as argument).
      // Bit 0x80 of the first byte indicates the presence of an optional
      // part; if present, bits 0x80 and 0x40 of the second byte determine
      // whether we have delay, command, or both. The possible forms are:
      //   0NNNNNNN                   - 1  byte:  note
      //   1NNNNNNN 0CCCCCCC          - 2+ bytes: note, command [data]
      //   1NNNNNNN 10DDDDDD          - 2  bytes: note, delay
      //   1NNNNNNN 11DDDDDD CCCCCCCC - 3+ bytes: note, delay, command [data]
      event.note = *stream & 0x7F;

      if (!(*stream++ & 0x80))
        break; // 1st byte is 0NNNNNNN: no delay or command; done.

      if (stream_end - stream < 1)
        return -1;
      if (*stream & 0x80) {
        // 2nd byte is 1xDDDDDD: delay byte present
        event.delay = *stream & 0x3F;

        if (!(*stream++ & 0x40))
          break; // 2nd byte is 10DDDDDD: no command; done.
      }

      // If we reach this point, a command byte follows.
      if (stream_end - stream < 1)
        return -1;

      if (0x40 <= *stream) {
        // command 0x40 or higher: Set Volume
        event.volume = *stream - 0x40 + 1; // masking needed?
        stream++;
      } else if ((0x20 <= *stream) && (*stream <= 0x3F)) {
        // command 0x20 to 0x3F: Set Instrument
        event.instrument = *stream - 0x20 + 1;
        stream++;
      } else {
        // command 0x1F or lower
        if (bmf.version == BMF0_9B) {
          // version 0.9b, command 0x1F or lower: ?
          stream++;
        } else if (bmf.version == BMF1_2) {
          /* 1.2 */
          if (0x01 <= *stream  && *stream <= 0x06) {
            if (stream_end - stream < 2)
              return -1;
            switch (*stream) {
            case 0x01: // Set Modulator Volume -> 0x01
              event.cmd = 0x01;
              event.cmd_data = stream[1];
              break;

            case 0x02: // ?
            case 0x03: // ?
              break;

            case 0x04: // Set Speed -> 0x10
              event.cmd = 0x10;
              event.cmd_data = stream[1];
              break;

            case 0x05: // Set Carrier Volume (port 380)
              event.volume = stream[1] + 1; // masking needed?
              break;

            case 0x06: // Set Carrier Volume (port 382)
              event.volume = stream[1] + 1; // masking needed?
              break;
            }
            stream += 2;
          } // if (0x01 <= *stream  && *stream <= 0x06)

        } // if (bmf.version == BMF1_2)

        // The following cases are not handled and don't even
        // increment the stream pointer (that's probably wrong):
        // * version 1.1, commands 0x1F or lower
        // * version 1.2, commands 0x00 and 0x07 to 0x1F

      } // ! if (0x40 <= *stream)

    } // switch

#ifdef DEBUG
    AdPlug_LogWrite("%02X %02X %02X %02X %02X %02X  <---- ",
                    event.note, event.delay, event.volume,
                    event.instrument, event.cmd, event.cmd_data);
    for (int zz = 0; zz < (stream - last); zz++)
      AdPlug_LogWrite(" %02X", last[zz]);
    AdPlug_LogWrite("\n");
    last = stream;
#endif
  } // for

  return stream - stream_start;
}
