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
 * nemuopl.h - Emulated OPL using the Nuked OPL3 emulator
 */

#ifndef H_ADPLUG_NEMUOPL
#define H_ADPLUG_NEMUOPL

#include "opl.h"

typedef struct _opl3_chip opl3_chip;

class CNemuopl: public Copl
{
public:
  CNemuopl(int rate);
  ~CNemuopl();

  void update(short *buf, int samples);

  void write(int reg, int val);

  void init();

private:
  opl3_chip*	opl;
  int			samplerate;
};

#endif
