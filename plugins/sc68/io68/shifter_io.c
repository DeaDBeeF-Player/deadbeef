/*
 *       sc68 - Atari ST shifter IO plugin (50/60hz and resolution)
 *         Copyright (C) 1998 Benjamin Gerard <ben@sashipa.com>
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

#include "emu68/struct68.h"
#if _DEBUG
# include "file68/debugmsg68.h"
#endif /*_DEBUG*/

static u8 shifter_0a = 0xfe;
static u8 shifter_60 = 0x00;

static u32 shifter_readB(u32 addr, cycle68_t cycle)
{
  addr &= 0xFF;
  cycle = cycle;
  if (addr == 0x0A) {
    return shifter_0a;
  } else if (addr == 0x60) {
    return shifter_60;
  }
  return 0;
}

static u32 shifter_readW(u32 addr, cycle68_t cycle)
{
  return (shifter_readB(addr,cycle)<<8) | shifter_readB(addr+1,cycle);
}

static u32 shifter_readL(u32 addr, cycle68_t cycle)
{
  return (shifter_readW(addr,cycle)<<16) | shifter_readW(addr+2,cycle+4);
}

static void shifter_writeB(u32 addr, u32 v, cycle68_t cycle)
{
  addr &= 0xFF;
  cycle = cycle;
  if (addr == 0x0A) {
#if _DEBUG
    SC68os_pdebug("Writing shifter %02X <= %02x\n",addr, v&255);
#endif
    shifter_0a = v;
  } else if (addr == 0x60) {
#if _DEBUG
    SC68os_pdebug("Writing shifter %02X <= %02x\n",addr, v&255);
#endif
    shifter_60 = v;
  }
}

static void shifter_writeW(u32 addr, u32 v, cycle68_t cycle)
{
  shifter_writeB(addr,   v>>8, cycle);
  shifter_writeB(addr+1, v,    cycle);
}

static void shifter_writeL(u32 addr, u32 v, cycle68_t cycle)
{
  shifter_writeW(addr,   v>>16, cycle);
  shifter_writeW(addr+2, v,     cycle+4);
}

static int68_t *shifter_int(cycle68_t cycle)
{
  cycle = cycle;
  return 0;
}

static u32 shifter_nextint(cycle68_t cycle)
{
  cycle = cycle;
  return IO68_NO_INT;
}

static int shifter_reset(void)
{
  shifter_0a = 0xfe;
  shifter_60 = 0x00;
  return 0;
}

static void shifter_subcycle(cycle68_t cycle)
{
  cycle = cycle;
}

io68_t shifter_io =
{
  0,
  "Shifter",
  0xFFFF8200, 0xFFFF82FF,
  {shifter_readB,  shifter_readW,  shifter_readL},
  {shifter_writeB, shifter_writeW, shifter_writeL},
  shifter_int, shifter_nextint,
  shifter_subcycle,
  shifter_reset,
  0,0
};
