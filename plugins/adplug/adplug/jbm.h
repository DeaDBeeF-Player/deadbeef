/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2007 Simon Peter <dn.tlp@gmx.net>, et al.
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
 * jbm.h - JBM Player by Dennis Lindroos <lindroos@nls.fi>
 */

#ifndef H_ADPLUG_JBMPLAYER
#define H_ADPLUG_JBMPLAYER

#include "player.h"

class CjbmPlayer: public CPlayer
{
 public:
  static CPlayer *factory(Copl *newopl);

  CjbmPlayer(Copl *newopl) : CPlayer(newopl), m(0), sequences(0)
    { }
  ~CjbmPlayer()
    { delete[] sequences; delete[] m; }

  bool load(const std::string &filename, const CFileProvider &fp);
  bool update();
  void rewind(int subsong);

  float getrefresh()
    { return timer; }

  std::string gettype()
    {
      return std::string(flags&1 ? "JBM Adlib Music [rhythm mode]" :
			 "JBM Adlib Music");
    }
  std::string getauthor()
    { return std::string("Johannes Bjerregaard"); }

 protected:

  unsigned char *m;
  float timer;
  unsigned short flags, voicemask;
  unsigned short seqtable, seqcount;
  unsigned short instable, inscount;
  unsigned short *sequences;
  unsigned char bdreg; 

  typedef struct {
    unsigned short trkpos, trkstart, seqpos;
    unsigned char seqno, note;
    short vol;
    short delay;
    short instr;
    unsigned char frq[2];
    unsigned char ivol, dummy;
  } JBMVoice;

  JBMVoice voice[11];

 private:
  //void calc_opl_frequency(JBMVoice *);
  void set_opl_instrument(int, JBMVoice *); 
  void opl_noteonoff(int, JBMVoice *, bool);
};

#endif
