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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 * adl.h - ADL player adaption by Simon Peter <dn.tlp@gmx.net>
 */

#ifndef H_ADPLUG_ADLPLAYER
#define H_ADPLUG_ADLPLAYER

#include <inttypes.h>

#include "player.h"

class AdlibDriver;

class CadlPlayer: public CPlayer
{
 public:
  static CPlayer *factory(Copl *newopl);

  CadlPlayer(Copl *newopl);
  ~CadlPlayer();

  bool load(const char *filename, const CFileProvider &fp);
  bool update();
  void rewind(int subsong = -1);

  // refresh rate is fixed at 72Hz
  float getrefresh()
    {
      return 72.0f;
    }

  unsigned int getsubsongs();
  unsigned int getsubsong() { return cursubsong; }
  const char *gettype() { return "Westwood ADL"; }

 private:
  int numsubsongs, cursubsong;

  AdlibDriver *_driver;

  uint8_t _trackEntries[120];
  uint8_t *_soundDataPtr;
  int _sfxPlayingSound;

  uint8_t _sfxPriority;
  uint8_t _sfxFourthByteOfSong;

  int _numSoundTriggers;
  const int *_soundTriggers;

  static const int _kyra1NumSoundTriggers;
  static const int _kyra1SoundTriggers[];

  bool init();
  void process();
  void playTrack(uint8_t track);
  void playSoundEffect(uint8_t track);
  void play(uint8_t track);
  void unk1();
  void unk2();
};

#endif
