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

  bool load(const std::string &filename, const CFileProvider &fp);
  bool update();
  void rewind(int subsong = -1);

  // refresh rate is fixed at 72Hz
  float getrefresh()
    {
      return 72.0f;
    }

  unsigned int getsubsongs();
  unsigned int getsubsong() { return cursubsong; }
  std::string gettype();

 private:
  int numsubsongs, cursubsong;

  AdlibDriver *_driver;

  uint8_t  _version;
  uint8_t  _trackEntries[120];
  uint16_t _trackEntries16[250];
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
  void playTrack(uint16_t track);
  void playSoundEffect(uint16_t track);
  void play(uint16_t track);
  void unk1();
  void unk2();
};

#endif
