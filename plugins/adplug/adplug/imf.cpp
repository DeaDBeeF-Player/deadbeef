/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2008 Simon Peter <dn.tlp@gmx.net>, et al.
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
 * imf.cpp - IMF Player by Simon Peter <dn.tlp@gmx.net>
 *
 * FILE FORMAT:
 * There seem to be 2 different flavors of IMF formats out there. One version
 * contains just the raw IMF music data. In this case, the first word of the
 * file is always 0 (because the music data starts this way). This is already
 * the music data! So read in the entire file and play it.
 *
 * If this word is greater than 0, it specifies the size of the following
 * song data in bytes. In this case, the file has a footer that contains
 * arbitrary infos about it. Mostly, this is plain ASCII text with some words
 * of the author. Read and play the specified amount of song data and display
 * the remaining data as ASCII text.
 *
 * NOTES:
 * This player handles the two above mentioned formats, as well as a third
 * type, invented by Martin Fernandez <mfernan@cnba.uba.ar>, that's got a
 * proper header to add title/game name information. After the header starts
 * the normal IMF file in one of the two above mentioned formats.
 *
 * This player also handles a special footer format by Adam Nielsen,
 * which has defined fields of information about the song, the author
 * and more.
 */

#include <cstring>

#include "imf.h"
#include "database.h"

/*** public methods *************************************/

CPlayer *CimfPlayer::factory(Copl *newopl)
{
  return new CimfPlayer(newopl);
}

bool CimfPlayer::load(const std::string &filename, const CFileProvider &fp)
{
  binistream *f = fp.open(filename);
  if (!f) return false;

  // file validation section
  unsigned long hdr_size = 0;
  {
    char	header[5];
    int		version;

    f->readString(header, 5);
    version = f->readInt(1);

    if (memcmp(header, "ADLIB", 5) || version != 1) {
      if (!fp.extension(filename, ".imf") && !fp.extension(filename, ".wlf")) {
	// It's no IMF file at all
	fp.close(f);
	return false;
      } else
	f->seek(0);	// It's a normal IMF file
    } else {
      // It's a IMF file with header
      track_name = f->readString('\0');
      game_name = f->readString('\0');
      f->ignore(1);
      hdr_size = f->pos();
    }
  }

  // determine data size
  unsigned long file_size = fp.filesize(f);
  int len_size = hdr_size ? 4 : 2;
  unsigned long song_size = f->readInt(len_size);

  if (!song_size) {	// raw music data (no length field, no footer)
    f->seek(-len_size, binio::Add);
    len_size = 0;
    song_size = file_size - hdr_size;
    // some IMF files end *before* the last time value (either format)
    if (song_size & 2) song_size += 2;
  }

  hdr_size += len_size;

  // validity check
  if (hdr_size + 4 > file_size || song_size & 3 // too short || invalid length
      || (song_size > file_size - hdr_size && // truncated song data
	  song_size != file_size + 2 - hdr_size)) { // allow trunc. final time
    fp.close(f);
    return false;
  }

  // read song data
  size = song_size / 4;
  data = new Sdata[size];
  for (unsigned long i = 0; i < size; i++) {
    data[i].reg = f->readInt(1);
    data[i].val = f->readInt(1);
    data[i].time = f->readInt(2);
  }

  // read footer, if any
  if (song_size < file_size - hdr_size) {
    unsigned long footerlen = file_size - hdr_size - song_size;
    char signature = f->readInt(1);

    if (signature == 0x1a && footerlen <= 1 + 3 * 256 + 9) {
      // Adam Nielsen's footer format
      track_name = f->readString();
      author_name = f->readString();
      remarks = f->readString();
      // f->ignore(9); // tagging program
    } else {
      // Generic footer
      footer = new char[footerlen + 1];
      footer[0] = signature;
      f->readString(&footer[1], footerlen);
      footer[footerlen] = '\0';	// Make ASCIIZ string

      if (footerlen == 88 && // maybe Muse tag data
	  !footer[17] && !footer[81] && track_name.empty()) {
	// For the lack of a better test assume it's Muse data if the size
	// is correct and both string fields end with NUL. Format is:
	//  2 Bytes: unknown
	// 16 Bytes: title
	// 64 Bytes: remarks (usually source file)
	//  6 Bytes: unknown data
	track_name = std::string(&footer[2]);
	remarks = std::string(&footer[18]);
	delete[] footer;
	footer = 0;
      }
    }
  }

  rate = getrate(filename, fp, f);
  fp.close(f);
  rewind(0);
  return true;
}

bool CimfPlayer::update()
{
	do {
		opl->write(data[pos].reg,data[pos].val);
		del = data[pos].time;
		pos++;
	} while(!del && pos < size);

	if(pos >= size) {
		pos = 0;
		songend = true;
	}
	else timer = rate / (float)del;

	return !songend;
}

void CimfPlayer::rewind(int subsong)
{
	pos = 0; del = 0; timer = rate; songend = false;
	opl->init(); opl->write(1,32);	// go to OPL2 mode
}

std::string CimfPlayer::gettitle()
{
  if (game_name.empty()) return track_name;
  if (track_name.empty()) return game_name;

  return track_name + " - " + game_name;
}

std::string CimfPlayer::getdesc()
{
  if (footer)
    return std::string(footer);

  return remarks;
}

/*** private methods *************************************/

float CimfPlayer::getrate(const std::string &filename, const CFileProvider &fp, binistream *f)
{
  if(db) {	// Database available
    f->seek(0, binio::Set);
    CClockRecord *record = (CClockRecord *)db->search(CAdPlugDatabase::CKey(*f));
    if (record && record->type == CAdPlugDatabase::CRecord::ClockSpeed)
      return record->clock;
  }

  // Otherwise the database is either unavailable, or there's no entry for this file
  if (fp.extension(filename, ".imf")) return 560.0f;
  if (fp.extension(filename, ".wlf")) return 700.0f;
  return 700.0f; // default speed for unknown files that aren't .IMF or .WLF
}
