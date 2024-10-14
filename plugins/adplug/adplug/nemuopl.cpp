/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2006 Simon Peter, <dn.tlp@gmx.net>, et al.
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
 * nemuopl.cpp - Emulated OPL using the Nuked OPL3 emulator
 */

#include "nemuopl.h"

extern "C" {
#include "nukedopl.h"
}

CNemuopl::CNemuopl(int rate)
{
  opl = new opl3_chip();
  OPL3_Reset(opl, rate);
  currType = TYPE_OPL3;
  samplerate = rate;
}

CNemuopl::~CNemuopl()
{
  delete opl;
}

void CNemuopl::update(short *buf, int samples)
{
  OPL3_GenerateStream(opl, buf, samples);
}

void CNemuopl::write(int reg, int val)
{
  OPL3_WriteRegBuffered(opl, (currChip << 8) | reg, val);
}

void CNemuopl::init()
{
  OPL3_Reset(opl, samplerate);
  currChip = 0;
}
