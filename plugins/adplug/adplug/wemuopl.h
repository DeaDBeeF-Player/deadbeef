/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2008 Simon Peter, <dn.tlp@gmx.net>, et al.
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
 * wemuopl.h - Emulated OPL using the DOSBox OPL3 emulator
 */

#ifndef H_ADPLUG_WEMUOPL
#define H_ADPLUG_WEMUOPL

#include "opl.h"
extern "C" {
#include "woodyopl.h"
}

class CWemuopl: public Copl
{
public:
  CWemuopl(int rate, bool bit16, bool usestereo)
    : use16bit(bit16), stereo(usestereo), sampleerate(rate)
    {
      opl.adlib_init(rate, usestereo ? 2 : 1, bit16 ? 2 : 1);
      currType = TYPE_OPL3;
    };

  void update(short *buf, int samples)
    {
      opl.adlib_getsample(buf, samples);
    }

  // template methods
  void write(int reg, int val)
    {
      opl.adlib_write((currChip << 8) | reg, val);
    };

  void init() {
    opl.adlib_init(sampleerate, stereo ? 2 : 1, use16bit ? 2 : 1);
    currChip = 0;
  };

private:
  OPLChipClass	opl;
  bool	use16bit,stereo;
  int	sampleerate;
};

#endif
