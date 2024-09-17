/*
  AdPlug - Replayer for many OPL2/OPL3 audio file formats.
  Copyright (C) 1999 - 2008 Simon Peter <dn.tlp@gmx.net>, et al.

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

  cff.cpp - BoomTracker loader by Riven the Mage <riven@ok.ru>
*/
/*
  NOTE: Conversion of slides is not 100% accurate. Original volume slides
  have effect on carrier volume only. Also, original arpeggio, frequency & volume
  slides use previous effect data instead of current.
*/

#include <cstring>

#include "cff.h"

/* -------- Public Methods -------------------------------- */

CPlayer *CcffLoader::factory(Copl *newopl)
{
  return new CcffLoader(newopl);
}

bool CcffLoader::load(const std::string &filename, const CFileProvider &fp)
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

  // '<CUD-FM-File>' - signed ?
  f->readString(header.id, sizeof(header.id));
  header.version = f->readInt(1);
  header.size = f->readInt(2);
  header.packed = f->readInt(1);
  f->ignore(sizeof(header.reserved));

  if (memcmp(header.id, "<CUD-FM-File>\x1A\xDE\xE0", sizeof(header.id)) ||
      header.size < 16 /* for unpacker's id; real minimum is bigger */) {
    fp.close(f);
    return false;
  }

  // always allocate padding needed for unpacker
  unsigned char *module = new unsigned char[header.size + 8];
  size_t module_size = header.size;
  f->readString((char *)module, header.size);
  fp.close(f);

  // unpack
  if (header.packed) {
      unsigned char *packed_module = module;
      memset(packed_module + header.size, 0, 8);
      module = new unsigned char[0x10000];

      cff_unpacker *unpacker = new cff_unpacker;
      module_size = unpacker->unpack(packed_module,module);
      delete unpacker;
      delete [] packed_module;
  }

  if (module_size < 0x669 + 64 * 9 * 3 ||
      memcmp(&module[0x5E1], "CUD-FM-File - SEND A POSTCARD -", 31)) {
    delete [] module;
    return false;
  }

  // init CmodPlayer
  realloc_instruments(47);
  realloc_order(64);
  realloc_patterns(36, 64, 9);
  init_notetable(conv_note);
  init_trackord();

  // load instruments
  for (int i = 0; i < 47; i++) {
    memcpy(&instruments[i], &module[i * 32], sizeof(cff_instrument) - 1);
    instruments[i].name[20] = 0;

    for (int j = 0; j < 11; j++)
      inst[i].data[conv_inst[j]] = instruments[i].data[j];
  }

  // number of patterns
  nop = module[0x5E0];
  if (nop < 1 || nop > 36 || (size_t)0x669 + nop * 64 * 9 * 3 > module_size) {
    delete [] module;
    return false;
  }

  // load title & author
  memcpy(song_title, &module[0x614], sizeof(song_title));
  memcpy(song_author, &module[0x600], sizeof(song_author));

  // load order
  memcpy(order, &module[0x628], 64);

  // load tracks
 cff_event (*events)[64][9] = (cff_event (*)[64][9]) &module[0x669];
  for (int i = 0, t = 0; i < nop; i++) {
      unsigned char old_event_byte2[9] = {};

      for (int j = 0; j < 9; j++, t++) {
	  for (int k = 0; k < 64; k++) {
	      cff_event *event = &events[i][k][j];

	      // convert note
	      if (event->byte0 == 0x6D)
		tracks[t][k].note = 127;
	      else
		tracks[t][k].note = event->byte0;

	      if (event->byte2)
		old_event_byte2[j] = event->byte2;

	      // convert effect
	      switch (event->byte1) {
		case 'I': // set instrument
		  if (event->byte2 < 47) // ignore invalid instruments
		    tracks[t][k].inst = event->byte2 + 1;
		  tracks[t][k].param1 = tracks[t][k].param2 = 0;
		  break;

		case 'H': // set tempo
		  tracks[t][k].command = 7;
		  if (event->byte2 < 16) {
		      tracks[t][k].param1 = 0x07;
		      tracks[t][k].param2 = 0x0D;
		  }
		  break;

		case 'A': // set speed
		  tracks[t][k].command = 19;
		  tracks[t][k].param1  = event->byte2 >> 4;
		  tracks[t][k].param2  = event->byte2 & 15;
		  break;

		case 'L': // pattern break
		  tracks[t][k].command = 13;
		  tracks[t][k].param1  = event->byte2 >> 4;
		  tracks[t][k].param2  = event->byte2 & 15;
		  break;

		case 'K': // order jump
		  tracks[t][k].command = 11;
		  tracks[t][k].param1  = event->byte2 >> 4;
		  tracks[t][k].param2  = event->byte2 & 15;
		  break;

		case 'M': // set vibrato/tremolo
		  tracks[t][k].command = 27;
		  tracks[t][k].param1  = event->byte2 >> 4;
		  tracks[t][k].param2  = event->byte2 & 15;
		  break;

		case 'C': // set modulator volume
		  tracks[t][k].command = 21;
		  tracks[t][k].param1 = (0x3F - event->byte2) >> 4;
		  tracks[t][k].param2 = (0x3F - event->byte2) & 15;
		  break;

		case 'G': // set carrier volume
		  tracks[t][k].command = 22;
		  tracks[t][k].param1 = (0x3F - event->byte2) >> 4;
		  tracks[t][k].param2 = (0x3F - event->byte2) & 15;
		  break;

		case 'B': // set carrier waveform
		  tracks[t][k].command = 25;
		  tracks[t][k].param1  = event->byte2;
		  tracks[t][k].param2  = 0x0F;
		  break;

		case 'E': // fine frequency slide down
		  tracks[t][k].command = 24;
		  tracks[t][k].param1  = old_event_byte2[j] >> 4;
		  tracks[t][k].param2  = old_event_byte2[j] & 15;
		  break;

		case 'F': // fine frequency slide up
		  tracks[t][k].command = 23;
		  tracks[t][k].param1  = old_event_byte2[j] >> 4;
		  tracks[t][k].param2  = old_event_byte2[j] & 15;
		  break;

		case 'D': // fine volume slide
		  tracks[t][k].command = 14;
		  if (old_event_byte2[j] & 15) {
		      // slide down
		      tracks[t][k].param1 = 5;
		      tracks[t][k].param2 = old_event_byte2[j] & 15;
		  } else {
		      // slide up
		      tracks[t][k].param1 = 4;
		      tracks[t][k].param2 = old_event_byte2[j] >> 4;
		  }
		  break;

		case 'J': // arpeggio
		  tracks[t][k].param1  = old_event_byte2[j] >> 4;
		  tracks[t][k].param2  = old_event_byte2[j] & 15;
		  break;
	      }
	  }
      }
  }

  delete [] module;

  // order loop
  restartpos = 0;

  // order length
  if (order[0] >= 0x36) // empty order list or invalid pattern
    return false;
  for (length = 1; length < 64; length++) {
    if (order[length] & 0x80) // end marker, keep length
      break;
    if (order[length] >= 36) // invalid pattern number
      return false;
  }

  // default tempo
  bpm = 0x7D;

  rewind(0);

  return true;	
}

void CcffLoader::rewind(int subsong)
{
  CmodPlayer::rewind(subsong);

  // default instruments
  for (int i = 0; i < 9; i++) {
      channel[i].inst = i;

      channel[i].vol1 = 63 - (inst[i].data[10] & 63);
      channel[i].vol2 = 63 - (inst[i].data[9] & 63);
  }
}

std::string CcffLoader::gettype()
{
  if (header.packed)
    return std::string("BoomTracker 4, packed");
  else
    return std::string("BoomTracker 4");
}

std::string CcffLoader::gettitle()
{
  return std::string(song_title, sizeof(song_title));
}

std::string CcffLoader::getauthor()
{
  return std::string(song_author, sizeof(song_author));
}

std::string CcffLoader::getinstrument(unsigned int n)
{
  if (n < getinstruments())
    return std::string(instruments[n].name);
  else
    return std::string();
}

unsigned int CcffLoader::getinstruments()
{
  return 47;
}

#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4244)
#pragma warning(disable:4018)
#endif

/*
  Lempel-Ziv-Tyr ;-)
*/
size_t CcffLoader::cff_unpacker::unpack(unsigned char *ibuf, unsigned char *obuf)
{
  if (memcmp(ibuf, "YsComp\007CUD1997\x1A\x04", 16))
    return 0;

  input = ibuf + 16;
  output = obuf;

  output_length = 0;

  heap = new unsigned char[0x10000];
  dictionary = new unsigned char *[0x8000];

  if(!start_block())
    goto fail;

  // LZW
  for (;;) {
    switch (unsigned long new_code = get_code()) {
    case 0x00: // end of data
      goto done;

    case 0x01: // end of block
      if (!start_block()) goto fail;
      break;

    case 0x02: // expand code length
      code_length++;
      if (code_length > 16) goto fail;
      break;

    case 0x03: { // RLE
	unsigned char repeat_length = get_code(2) + 1;

	unsigned char length = 4 << get_code(2);
	size_t repeat_counter = get_code(length);

	size_t end = output_length + repeat_counter * repeat_length;
	if (repeat_length > output_length ||
	    repeat_counter > 0x10000 || end > 0x10000)
	  goto fail;

	while (output_length < end)
	  put_string(&output[output_length - repeat_length], repeat_length);

	if (!start_string()) goto fail;
      }
      break;

    default:
      if (new_code >= 0x104 + dictionary_length) {
	  // dictionary <- old.code.string + old.code.char
	  the_string[++the_string[0]] = the_string[1];
      } else {
	  // dictionary <- old.code.string + new.code.char
	  unsigned char temp_string[256];

	  translate_code(new_code, temp_string);

	  the_string[++the_string[0]] = temp_string[1];
      }

      expand_dictionary(the_string);

      // output <- new.code.string
      translate_code(new_code, the_string);
      if (!put_string()) goto fail;
      break;
    }
  }
 fail:
  output_length = 0;
 done:
  delete [] heap;
  delete [] dictionary;
  return output_length;
}

/* -------- Private cff_unpacker Methods ------------------------------- */

unsigned long CcffLoader::cff_unpacker::get_code(unsigned char bitlength)
{
  unsigned long code;
  // shifts of bits_buffer by 32 can be undefined (with 32 bit long)
  unsigned long long bits = bits_buffer;

  while (bits_left < bitlength) {
    bits |= (unsigned long long)*input++ << bits_left;
    bits_left += 8;
  }

  code = bits & ((1ULL << bitlength) - 1);

  bits_buffer = (unsigned long)(bits >> bitlength);
  bits_left -= bitlength;

  return code;
}

void CcffLoader::cff_unpacker::translate_code(unsigned long code, unsigned char *string)
{
  if (code >= 0x104 + dictionary_length) { // invalid code
    string[0] = string[1] = 0;
  } else if (code >= 0x104) { // dictionary entry
    memcpy(string, dictionary[code - 0x104], dictionary[code - 0x104][0] + 1);
  } else { // single character
    string[0] = 1;
    string[1] = (code - 4) & 0xFF;
  }
}

bool CcffLoader::cff_unpacker::put_string(unsigned char *string, size_t length)
{
  if (output_length + length > 0x10000) return false;

  memcpy(&output[output_length], string, length);
  output_length += length;

  return true;
}

bool CcffLoader::cff_unpacker::start_block()
{
  code_length = 9;

  bits_buffer = 0;
  bits_left = 0;

  heap_length = 0;
  dictionary_length = 0;

  return start_string();
}

bool CcffLoader::cff_unpacker::start_string()
{
  translate_code(get_code(), the_string);
  return put_string();
}

void CcffLoader::cff_unpacker::expand_dictionary(unsigned char *string)
{
  if (string[0] >= 0xF0 || heap_length + string[0] + 1 > 0x10000)
    return;

  memcpy(&heap[heap_length], string, string[0] + 1);
  dictionary[dictionary_length++] = &heap[heap_length];
  heap_length += string[0] + 1;
}

#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(default:4244)
#pragma warning(default:4018)
#endif
