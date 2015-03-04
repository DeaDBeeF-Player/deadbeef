/*
 *                sc68 - Paula IO plugin (AMIGA soundchip)
 *         Copyright (C) 1999 Benjamin Gerard <ben@sashipa.com>
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

#include "io68/paula_io.h"
#include "io68/paulaemul.h"

#ifdef _DEBUG
# include "file68/debugmsg68.h"
#endif /*_DEBUG*/

static void reload(paulav_t *v , const u8 * p);

static unsigned int clearset(const unsigned int v, const unsigned int clrset)
{
  if (clrset & 0x8000) {
    return (v | clrset) & 0x7FFF;
  } else {
    return v & ~clrset;
  }
}

static int DMACON(const int dmacon)
{
  return (dmacon << (8*sizeof(int)-1-9) >> (8*sizeof(int)-1) )
    & dmacon & 0xF;
}

static int INTREQ(const int intreq)
{
  return intreq & (0xF<<7);
}

static int INTENA(const int intena)
{
  return (intena << (8*sizeof(int)-1-14) >> (8*sizeof(int)-1))
    & intena & (0xF<<7);
}


static u32 paula_readB(u32 addr, cycle68_t cycle)
{
  addr &= 0xFF;
  switch(addr) {
    case PAULA_VHPOSR:
      return paula[PAULA_VHPOSR]++;
  default:
#ifdef _DEBUG
    SC68os_pdebug("Read Byte in AMIGA HW (%02X)", addr);
#endif

    /* Copy shadow registers. */
    paula[PAULA_DMACONRH] = (paula_dmacon>>8)&0x7f;
    paula[PAULA_DMACONRL] = paula_dmacon;

    paula[PAULA_INTENARH] = (paula_intena>>8)&0x7f;
    paula[PAULA_INTENARL] = paula_intena;

    paula[PAULA_INTREQRH] = (paula_intreq>>8)&0x7f;
    paula[PAULA_INTREQRL] = paula_intreq;

    paula[PAULA_ADKCONRH] = (paula_adkcon>>8)&0x7f;
    paula[PAULA_ADKCONRL] = paula_adkcon;

    return paula[addr];
  }
}

static u32 paula_readW(u32 addr, cycle68_t cycle)
{
  addr &= 0xFF;
  switch (addr) {
  case PAULA_DMACONR:
    return paula_dmacon & 0x7fff;
  case PAULA_INTENAR:
    return paula_intena & 0x7fff;
  case PAULA_INTREQR:
    return paula_intreq & 0x7fff;
  case PAULA_ADKCON:
    return paula_adkcon & 0x7fff;
  default:
#ifdef _DEBUG
    SC68os_pdebug("Read Word in AMIGA HW (%02X)", addr);
#endif
    return (paula[addr]<<8) | paula[addr+1];
  }
}

static u32 paula_readL(u32 addr, cycle68_t cycle)
{
  return (paula_readW(addr,cycle)<<16) | paula_readW(addr+2,cycle+4);
}

/* start DMA on voice n ( DMA disable -> enable ) */
static void start_DMA(int bit)
{
  int old,chg,cur;

  old = DMACON(paula_dmacon);
  paula_dmacon |= bit;
  cur = DMACON(paula_dmacon);
  chg = cur & ~old;

  if(chg&1) reload(paulav+0, paula+0xA0);
  if(chg&2) reload(paulav+1, paula+0xB0);
  if(chg&4) reload(paulav+2, paula+0xC0);
  if(chg&8) reload(paulav+3, paula+0xD0);
}

/* stop DMA bit to be stopped */
static void stop_DMA(int bit)
{
  paula_dmacon &= ~bit;
  //	audio_int(0x8000 | ((bit^0xF)<<7));

  /*	printf("STOP(%X)\n",bit&0x20f);
   */

}

/* Reload paula internal register with current value */
static void reload(paulav_t * v, const u8 * p)
{
  int len;

  v->start = v->adr =
    (((p[1]<<16)|(p[2]<<8)|p[3])&0x7FFFE) << PAULA_CT_FIX;
  len = (p[4]<<8) | p[5];
  len |= (!len) << 16;
  len <<= (PAULA_CT_FIX+1); 
  v->end = v->start + len;
}


/* Write INTREQ :
 *
 * - If clearing bits just release the interrupt. Nothing more to do.
 * - If setting bit checks wheter the interrupt is denied or not.
 *   When denied it seems that the internal pointer and length register
 *   are reloaded however its is not an official practice.
 */
static void write_intreq(const int intreq)
{
  int intdenied;

  if ( !(intreq & 0x8000) ) {
    /* Clearing ... */
    paula_intreq &= ~intreq;
    return;
  }

  /* Master interrupt not set : DENIED */
  intdenied = ~INTENA(paula_intena);
  /* Already requested : DENIED */
  intdenied |= paula_intreq;
  /* Only interrested by requested bits */
  intdenied &= intreq;
  /* Reload for each denied channel */

  /* $$$ May be should not reload if DMA is OFF $$$ */

  if(intdenied & (1<< 7)) reload(paulav+0, paula+0xA0);
  if(intdenied & (1<< 8)) reload(paulav+1, paula+0xB0);
  if(intdenied & (1<< 9)) reload(paulav+2, paula+0xC0);
  if(intdenied & (1<<10)) reload(paulav+3, paula+0xD0);

  paula_intreq |= intreq;
}

static void paula_writeB(u32 addr, u32 v, cycle68_t cycle)
{
  addr=addr; cycle=cycle;
  addr &= 0xFF;
  v    &= 0xFF;
  paula[addr] = v;

#if _DEBUG
  SC68os_pdebug("Write Byte in AMIGA HW (%02X <= %02x)", addr, v);
#endif
  
}

static void paula_writeW(u32 addr, u32 v, cycle68_t cycle)
{
  addr &= 0xFF;
  v &= 0xFFFF;

  /* Copy into hw-reg */ 
  paula[addr] = v >> 8;
  paula[(addr+1) & 0xFF] = v;

  switch (addr) {
  case PAULA_ADKCON:
    {
      int old_adkcon = paula_adkcon;
      paula_adkcon = clearset(old_adkcon, v);
#ifdef _DEBUG
      if (paula_adkcon & ~old_adkcon & 0xFF) {
	SC68os_pdebug("Amiga Audio Modulation (%02X)\n", paula_adkcon & 0xFF);
      }
#endif      
    } break;

  case PAULA_DMACON:
    {
      int old_dmacon = paula_dmacon;
      int old_dmaena = DMACON(old_dmacon);
      int paula_dmaena;
      int start;
      
      paula_dmacon = clearset(old_dmacon, v);
      paula_dmaena = DMACON(paula_dmacon);

      start = paula_dmaena & ~old_dmaena;

      if(start&1) reload(paulav+0, paula+0xA0);
      if(start&2) reload(paulav+1, paula+0xB0);
      if(start&4) reload(paulav+2, paula+0xC0);
      if(start&8) reload(paulav+3, paula+0xD0);

    } break;

  case PAULA_INTENA:
    {
      int old_intena = INTENA(paula_intena), new_intena;
      old_intena=old_intena;
      paula_intena = clearset(paula_intena, v);
      new_intena = INTENA(paula_intena);

#ifdef _DEBUG
      if ( new_intena & ~old_intena ) {
	SC68os_pdebug("Amiga Audio IRQ enabled (%X)\n",new_intena);
      }
#endif
    } break;

  case PAULA_INTREQ:
    write_intreq(v);
    break;

  default:
#if _DEBUG
    if (addr < PAULA_VOICE(0) || addr >= PAULA_VOICE(4)) {
      SC68os_pdebug("Write Word in AMIGA HW (%02X <= %04x)", addr, v);
    }
#endif
    break; /* to please vc7*/
  }
}

static void paula_writeL(u32 addr, u32 v, cycle68_t cycle)
{
  paula_writeW(addr,   v>>16, cycle);
  paula_writeW(addr+2, v,     cycle);
}

static int68_t *paula_int(cycle68_t cycle)
{
  cycle = cycle;
  return 0;
}

static cycle68_t paula_nextint(cycle68_t cycle)
{
  cycle = cycle;
  return IO68_NO_INT;
}

static int paula_reset(void)
{
  PL_reset();
  return 0;
}

static void paula_subcycle(cycle68_t sub)
{
  sub = sub;
}

io68_t paula_io = {
  NULL,
  "AMIGA Paula",
  0xFFDFF000, 0xFFDFF0DF,
  {paula_readB,  paula_readW,  paula_readL},
  {paula_writeB, paula_writeW, paula_writeL},
  paula_int, paula_nextint,
  paula_subcycle,
  paula_reset,
  0,0
};
