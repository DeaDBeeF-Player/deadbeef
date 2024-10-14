/*
  Adplug - Replayer for many OPL2/OPL3 audio file formats.
  Copyright (C) 1999 - 2007 Simon Peter <dn.tlp@gmx.net>, et al.

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

  fmc.cpp - FMC Loader by Riven the Mage <riven@ok.ru>
*/

#include <cstring>
#include "fmc.h"

/* -------- Public Methods -------------------------------- */

CPlayer *CfmcLoader::factory(Copl *newopl)
{
  return new CfmcLoader(newopl);
}

bool CfmcLoader::load(const std::string &filename, const CFileProvider &fp)
{
  binistream *f = fp.open(filename); if(!f) return false;
  const unsigned char conv_fx[16] = {0,1,2,3,4,8,255,255,255,255,26,11,12,13,14,15};

  int i,j,k,t=0;

  // read header
  f->readString(header.id, 4);
  f->readString(header.title, 21);
  header.title[20] = 0;
  header.numchan = f->readInt(1);

  // 'FMC!' - signed ?
  if (memcmp(header.id, "FMC!", 4) ||
      header.numchan < 1 || header.numchan > 32) {
    fp.close(f); return false;
  }

  // init CmodPlayer
  realloc_instruments(32);
  realloc_order(256);
  realloc_patterns(64,64,header.numchan);
  init_trackord();

  // load order
  for(i = 0; i < 256; i++) order[i] = f->readInt(1);

  f->ignore(2);

  // load instruments
  for(i = 0; i < 32; i++) {
    instruments[i].synthesis = f->readInt(1);
    instruments[i].feedback = f->readInt(1);

    instruments[i].mod_attack = f->readInt(1);
    instruments[i].mod_decay = f->readInt(1);
    instruments[i].mod_sustain = f->readInt(1);
    instruments[i].mod_release = f->readInt(1);
    instruments[i].mod_volume = f->readInt(1);
    instruments[i].mod_ksl = f->readInt(1);
    instruments[i].mod_freq_multi = f->readInt(1);
    instruments[i].mod_waveform = f->readInt(1);
    instruments[i].mod_sustain_sound = f->readInt(1);
    instruments[i].mod_ksr = f->readInt(1);
    instruments[i].mod_vibrato = f->readInt(1);
    instruments[i].mod_tremolo = f->readInt(1);

    instruments[i].car_attack = f->readInt(1);
    instruments[i].car_decay = f->readInt(1);
    instruments[i].car_sustain = f->readInt(1);
    instruments[i].car_release = f->readInt(1);
    instruments[i].car_volume = f->readInt(1);
    instruments[i].car_ksl = f->readInt(1);
    instruments[i].car_freq_multi = f->readInt(1);
    instruments[i].car_waveform = f->readInt(1);
    instruments[i].car_sustain_sound = f->readInt(1);
    instruments[i].car_ksr = f->readInt(1);
    instruments[i].car_vibrato = f->readInt(1);
    instruments[i].car_tremolo = f->readInt(1);

    instruments[i].pitch_shift = f->readInt(1);

    f->readString(instruments[i].name, 21);
    instruments[i].name[20] = 0;
  }

  // load tracks
  for (i=0;i<64;i++)
    {
      if(f->ateof()) break;

      for (j=0;j<header.numchan;j++)
	{
	  for (k=0;k<64;k++)
	    {
	      fmc_event event;

	      // read event
	      event.byte0 = f->readInt(1);
	      event.byte1 = f->readInt(1);
	      event.byte2 = f->readInt(1);

	      // convert event
	      tracks[t][k].note = event.byte0 & 0x7F;
	      tracks[t][k].inst = ((event.byte0 & 0x80) >> 3) + (event.byte1 >> 4) + 1;
	      tracks[t][k].command = conv_fx[event.byte1 & 0x0F];
	      tracks[t][k].param1 = event.byte2 >> 4;
	      tracks[t][k].param2 = event.byte2 & 0x0F;

	      // fix effects
	      if (tracks[t][k].command == 0x0E)   // 0x0E (14): Retrig
		tracks[t][k].param1 = 3;
	      if (tracks[t][k].command == 0x1A) { // 0x1A (26): Volume Slide
		if (tracks[t][k].param1 > tracks[t][k].param2)
		  {
		    tracks[t][k].param1 -= tracks[t][k].param2;
		    tracks[t][k].param2 = 0;
		  }
		else
		  {
		    tracks[t][k].param2 -= tracks[t][k].param1;
		    tracks[t][k].param1 = 0;
		  }
	      }
	    }

	  t++;
	}
    }
  fp.close(f);

  // convert instruments
  for (i=0;i<31;i++)
    buildinst(i);

  // data for Protracker
  activechan = (0xffffffffUL >> (32 - header.numchan)) << (32 - header.numchan);
  nop = t / header.numchan;
  if (!nop) return false; // truncated file, no track data
  restartpos = 0;

  // order length
  for (length = 0; length < 256; length++) {
    if (order[length] >= 0xFE) // end marker
      break;
    else if (order[length] >= nop /* or 64*/) // invalid pattern
      return false;
  }

  // flags
  flags = Faust;

  rewind(0);

  return true;
}

float CfmcLoader::getrefresh()
{
  return 50.0f;
}

std::string CfmcLoader::gettype()
{
  return std::string("Faust Music Creator");
}

std::string CfmcLoader::gettitle()
{
  return std::string(header.title);
}

std::string CfmcLoader::getinstrument(unsigned int n)
{
  return n < 32 ? std::string(instruments[n].name) : std::string();
}

unsigned int CfmcLoader::getinstruments()
{
  return 32;
}

/* -------- Private Methods ------------------------------- */

void CfmcLoader::buildinst(unsigned char i)
{
  inst[i].data[0]   = ((instruments[i].synthesis & 1) ^ 1);
  inst[i].data[0]  |= ((instruments[i].feedback & 7) << 1);

  inst[i].data[3]   = ((instruments[i].mod_attack & 15) << 4);
  inst[i].data[3]  |=  (instruments[i].mod_decay & 15);
  inst[i].data[5]   = ((15 - (instruments[i].mod_sustain & 15)) << 4);
  inst[i].data[5]  |=  (instruments[i].mod_release & 15);
  inst[i].data[9]   =  (63 - (instruments[i].mod_volume & 63));
  inst[i].data[9]  |= ((instruments[i].mod_ksl & 3) << 6);
  inst[i].data[1]   =  (instruments[i].mod_freq_multi & 15);
  inst[i].data[7]   =  (instruments[i].mod_waveform & 3);
  inst[i].data[1]  |= ((instruments[i].mod_sustain_sound & 1) << 5);
  inst[i].data[1]  |= ((instruments[i].mod_ksr & 1) << 4);
  inst[i].data[1]  |= ((instruments[i].mod_vibrato & 1) << 6);
  inst[i].data[1]  |= ((instruments[i].mod_tremolo & 1) << 7);

  inst[i].data[4]   = ((instruments[i].car_attack & 15) << 4);
  inst[i].data[4]  |=  (instruments[i].car_decay & 15);
  inst[i].data[6]   = ((15 - (instruments[i].car_sustain & 15)) << 4);
  inst[i].data[6]  |=  (instruments[i].car_release & 15);
  inst[i].data[10]  =  (63 - (instruments[i].car_volume & 63));
  inst[i].data[10] |= ((instruments[i].car_ksl & 3) << 6);
  inst[i].data[2]   =  (instruments[i].car_freq_multi & 15);
  inst[i].data[8]   =  (instruments[i].car_waveform & 3);
  inst[i].data[2]  |= ((instruments[i].car_sustain_sound & 1) << 5);
  inst[i].data[2]  |= ((instruments[i].car_ksr & 1) << 4);
  inst[i].data[2]  |= ((instruments[i].car_vibrato & 1) << 6);
  inst[i].data[2]  |= ((instruments[i].car_tremolo & 1) << 7);

  inst[i].slide     =   instruments[i].pitch_shift;
}
