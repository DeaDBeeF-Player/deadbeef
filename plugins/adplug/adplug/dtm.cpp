/*
  Adplug - Replayer for many OPL2/OPL3 audio file formats.
  Copyright (C) 1999 - 2006 Simon Peter, <dn.tlp@gmx.net>, et al.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

  dtm.cpp - DTM loader by Riven the Mage <riven@ok.ru>
*/
/*
  NOTE: Panning (Ex) effect is ignored.
*/

#include <algorithm>
#include <cstring>
#include "dtm.h"

/* -------- Public Methods -------------------------------- */

CPlayer *CdtmLoader::factory(Copl *newopl)
{
  return new CdtmLoader(newopl);
}

bool CdtmLoader::load(const std::string &filename, const CFileProvider &fp)
{
  static const unsigned char conv_inst[11] = {
    2, 1, 10, 9, 4, 3, 6, 5, 0, 8, 7
  };
  static const unsigned short conv_note[12] = {
    0x16B, 0x181, 0x198, 0x1B0, 0x1CA, 0x1E5,
    0x202, 0x220, 0x241, 0x263, 0x287, 0x2AE
  };

  binistream *f = fp.open(filename);
  if (!f) return false;

  // read header
  f->readString(header.id, sizeof(header.id));
  header.version = f->readInt(1);
  f->readString(header.title, sizeof(header.title));
  f->readString(header.author, sizeof(header.author));
  // Ensure title and author are NUL terminated. We may overwrite the
  // last character because the arrays are too short.
  header.author[sizeof(header.author) - 1] =
    header.title[sizeof(header.title) - 1] = 0;
  header.numpat = f->readInt(1);
  header.numinst = f->readInt(1) + 1;

  // check header
  if (memcmp(header.id, "DeFy DTM ", 9) || // signature exists?
      header.version != 0x10 || // good version?
      header.numinst > MAX_INST ||
      header.numinst < N_CHAN || // need a default instrument for each channel
      header.numpat == 0 ||
      f->error()) {
    fp.close (f);
    return false;
  }

  // load description
  memset(desc, 0, sizeof(desc));

  char *bufstr = desc;

  for (int i = 0; i < DESC_ROWS; i++) {
      // get line length
      unsigned char bufstr_length = f->readInt(1);

      if (bufstr_length > DESC_COLS) {
        // "desc" is too small to hold DESC_ROWS lines with DESC_COLS chars
        // each plus DESC_ROWS newlines and a NUL terminator. Accept a line
        // length of DESC_COLS anyway and truncate the last line if necessary.
        // Maybe we should grow desc or allocate it dynamically instead.
	fp.close(f);
	return false;
      }

      int max_length = desc + sizeof(desc) - 1 - bufstr;
      int discard = bufstr_length > max_length ? bufstr_length - max_length : 0;
      bufstr_length -= discard;

      // read line
      if (bufstr_length) {
	  f->readString(bufstr,bufstr_length);

	  for (int j = 0; j < bufstr_length; j++)
	    if (!bufstr[j])
	      bufstr[j] = 0x20;
          bufstr += bufstr_length;

	  if (discard)
            f->ignore(discard);
      }
      if (bufstr_length < max_length)
        *(bufstr++) = '\n';
  }
  *bufstr = 0;

  // init CmodPlayer
  realloc_instruments(header.numinst);
  realloc_order(N_ORD);
  realloc_patterns(header.numpat, N_ROW, N_CHAN);
  init_notetable(conv_note);
  init_trackord();

  // load instruments
  for (int i = 0; i < header.numinst; i++) {
      unsigned char name_length = f->readInt(1);

      if (name_length >= sizeof(instruments[i].name)) {
	fp.close(f);
	return false; // or truncate the name instead?
      }

      if (name_length)
	f->readString(instruments[i].name, name_length);

      instruments[i].name[name_length] = 0;

      f->readString((char *)instruments[i].data, sizeof(instruments[i].data));

      for (unsigned int j = 0; j < sizeof(conv_inst); j++)
	inst[i].data[conv_inst[j]] = instruments[i].data[j];
  }

  // load order
  f->readString((char *)order, N_ORD);

  // load tracks
  dtm_event pattern[N_ROW][N_CHAN];
  nop = header.numpat;
  for (int t = 0, i = 0; i < nop; i++) {
      unsigned short packed_length = f->readInt(2);
      if (!unpack_pattern(f, packed_length, pattern, sizeof(pattern))) {
	  fp.close(f);
	  return false;
      }

      // convert pattern
      for (int j = 0; j < N_CHAN; j++, t++) {
	  for (int k = 0; k < N_ROW; k++) {
	      dtm_event *event = &pattern[k][j];

	      // instrument
	      if (event->byte0 == 0x80) {
		if (event->byte1 < header.numinst) // not <= 0x80 !
		    tracks[t][k].inst = event->byte1 + 1;
	      }

	      // note + effect
	      else {
		  tracks[t][k].note = event->byte0;

		  if ((event->byte0 != 0) && (event->byte0 != 127))
		    tracks[t][k].note++;

		  // convert effects
		  unsigned char ev_param = event->byte1 & 0x0F;
		  switch (event->byte1 >> 4) {
		    case 0x0: // pattern break
		      if (ev_param == 1)
			tracks[t][k].command = 13;
		      break;

		    case 0x1: // freq. slide up
		      tracks[t][k].command = 28;
		      tracks[t][k].param1 = ev_param;
		      break;

		    case 0x2: // freq. slide down
		      tracks[t][k].command = 28;
		      tracks[t][k].param2 = ev_param;
		      break;

		    case 0xA: // set carrier volume
		    case 0xC: // set instrument volume
		      tracks[t][k].command = 22;
		      tracks[t][k].param1 = (0x3F - ev_param) >> 4; // always 3
		      tracks[t][k].param2 = (0x3F - ev_param) & 0xF;
		      break;

		    case 0xB: // set modulator volume
		      tracks[t][k].command = 21;
		      tracks[t][k].param1 = (0x3F - ev_param) >> 4; // always 3
		      tracks[t][k].param2 = (0x3F - ev_param) & 0xF;
		      break;

		    case 0xE: // set panning
		      break;

		    case 0xF: // set speed
		      tracks[t][k].command = 13;
		      tracks[t][k].param2 = ev_param;
		      break;
		  }
	      }
	  }
      }
  }

  if (f->error()) {
      fp.close(f);
      return false;
  }
  fp.close(f);

  // order length
  length = N_ORD;
  for (unsigned int i = 0; i < N_ORD; i++) {
      if (order[i] & 0x80) {
	  length = i;

	  if (order[i] == 0xFF)
	    restartpos = 0;
	  else
	    restartpos = order[i] - 0x80;

	  if (restartpos >= i) // bad restart position or empty order list
	    return false;

	  break;
      } else if (order[i] >= nop) {
	return false;
      }
  }

  // initial speed
  initspeed = 2;

  rewind(0);

  return true;
}

void CdtmLoader::rewind(int subsong)
{
  CmodPlayer::rewind(subsong);

  // default instruments
  for (int i = 0; i < N_CHAN; i++) {
      channel[i].inst = i;

      channel[i].vol1 = 63 - (inst[i].data[10] & 63);
      channel[i].vol2 = 63 - (inst[i].data[9] & 63);
  }
}

float CdtmLoader::getrefresh()
{
  return 18.2f;
}

std::string CdtmLoader::gettype()
{
  return std::string("DeFy Adlib Tracker");
}

std::string CdtmLoader::gettitle()
{
  return std::string(header.title);
}

std::string CdtmLoader::getauthor()
{
  return std::string(header.author);
}

std::string CdtmLoader::getdesc()
{
  return std::string(desc);
}

std::string CdtmLoader::getinstrument(unsigned int n)
{
  return n < header.numinst ? std::string(instruments[n].name) : std::string();
}

unsigned int CdtmLoader::getinstruments()
{
  return header.numinst;
}

/* -------- Private Methods ------------------------------- */

bool CdtmLoader::unpack_pattern(binistream *f, size_t ilen,
				void *obuf, size_t olen)
{
  unsigned char *outp = (unsigned char *)obuf;

  // RLE
  while (ilen--) {
    size_t repeat_counter = 1;
    unsigned char repeat_byte = f->readInt(1);

    if ((repeat_byte & 0xF0) == 0xD0) {
      if (!ilen--) return false; // truncated input

      repeat_counter = repeat_byte & 0x0F;
      repeat_byte = f->readInt(1);
    }

    // Attempts to generate too much data are normal, ignore the excess data.
    repeat_counter = std::min(repeat_counter, olen);

    memset(outp, repeat_byte, repeat_counter);
    outp += repeat_counter;
    olen -= repeat_counter;
  }

  return olen == 0 && !f->error(); // generated enough data?
}
