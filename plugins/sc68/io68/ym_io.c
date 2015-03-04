/*
 *                        sc68 - YM-2149 io plugin
 *           Copyright (C) 1999 Benjamin Gerard <ben@sashipa.com>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */ 

#include "io68/ym_io.h"
#include "io68/ymemul.h"

static u32 ym_readB(u32 addr, cycle68_t cycle)
{
  if (addr&3) return 0;
  /* Data register [ control ] */
  return YM_readreg(ym.ctrl,cycle);
}

static u32 ym_readW(u32 addr, cycle68_t cycle)
{
  if(addr&3) return 0;
  return YM_readreg(ym.ctrl,cycle)<<8;
}

static u32 ym_readL(u32 addr, cycle68_t cycle)
{
  return (ym_readW(addr,cycle)<<16) | ym_readW(addr+2,cycle);
}

static void ym_writeB(u32 addr, u32 v, cycle68_t cycle)
{
  addr &= 2;
  /* control register */
  if(!addr) ym.ctrl = v;
  /* Data register [ control ] */
  else YM_writereg(ym.ctrl,(u8)v,cycle);
}

static void ym_writeW(u32 addr, u32 v, cycle68_t cycle)
{
  ym_writeB(addr,   v>>8, cycle);
  /* Yes, that's really like this ! */
  /* ym_writeB(addr+1, v,    cycle); */
}

static void ym_writeL(u32 addr, u32 v, cycle68_t cycle)
{
  ym_writeW(addr,   v>>16, cycle);
  ym_writeW(addr+2, v,     cycle); /* !! Could add 4 cycles */
}

static int68_t *ym_int(cycle68_t cycle)
{
  cycle = cycle;
  return 0;
}

static cycle68_t ym_nextint(cycle68_t cycle)
{
  cycle = cycle;
  return IO68_NO_INT;
}

static int ym_reset(void)
{
  YM_reset();
  return 0;
}


io68_t ym_io =
{
  0,
  "YM-2149",
  0xFFFF8800, 0xFFFF88FF,
  {ym_readB,  ym_readW,  ym_readL},
  {ym_writeB, ym_writeW, ym_writeL},
  ym_int, ym_nextint,
  YM_subcycle,
  ym_reset,
  4,4
};
