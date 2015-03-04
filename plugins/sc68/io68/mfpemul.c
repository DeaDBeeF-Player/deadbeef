/*
 *                         sc68 - MFP 68901 emulator
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
 
#include <stdlib.h>
#include <config68.h>
#include "io68/mfpemul.h"

#define MFP_VECTOR_BASE ((mfp[0x17]>>4)<<6)
#define SEI             ((mfp[0x17]&8))
#define AEI             (!SEI)

/* Define this for more accurate emulation. */
/*#define _MFPIO68_SUPER_EMUL_*/

/* About cycles.
 *
 * The MFP use its own crystal clock. Its frequency is not a multiple
 * of the 68K one (8Mhz).
 * In order to convert 68K cycles to MFP ones the emulator use an internal
 * cycle unit : a "BOGO" cycle.
 * -1 "BOGO" cycle => 192 "8mhz 68K" cycles
 * -1 "BOGO" cycle => 625 "mfp" cycle
 */

/* Timer struct */
typedef struct
{
  /* Constant */

  u32 vector;     /* Interrupt vector.                         */
  u8  level;      /* Interrupt level.                          */
  u8  bit;        /* Bit mask for Interrupt stuffes registers. */
  u8  channel;    /* Channel A=0 or B=2.                       */

  /* Variable */

  u32 udc;        /* Last BOGO cycle registers have been updated. */
  u32 cti;        /* BOGO cycle when timer interrupt.             */

  u32 psc_cnt;    /* Current number of BOGO cycle in prescaler counter. */
  u32 psc_width;  /* Prescaler width (BOGO).                            */

  u32 cpp;        /* Number of MFP cycles per period (0:stopped).       */

  u32 tdr;        /* Timer Data register current value.         */
  u32 tdr2;       /* Timer Data register reset value.           */
  u32 tcr;        /* Timer control register (prescaler) [0..7]. */

} MFPtimer68_t;

static MFPtimer68_t timers[4] =
{
/*               Vector level bit   chan */
/* Timer A */  { 0x34,  6,    1<<5, 0    },
/* Timer B */  { 0x20,  6,    1<<0, 0    },
/* Timer C */  { 0x14,  6,    1<<5, 2    },
/* Timer D */  { 0x10,  6,    1<<4, 2    },
};

/* Shadow register. */
u8 mfp[0x40];

/* Next interrupt timer (0 is none). */
static MFPtimer68_t *mfp_itimer;

/* MFP prediv value in "BOGO" */
static int f[8] = { 0*625, 4*625, 10*625, 16*625, 50*625,
                    64*625, 100*625, 200*625 };

static MFPtimer68_t *find_next_int(void)
{
  int cti;
  MFPtimer68_t *t,*itimer;

  /* Look for timer that interrupt first */
  for(itimer=0L, t=timers, cti=0x7FFFFFFF; t<timers+4; t++) {
    /* $$$ This is not exact, timer could be stopped after a pending
            interrupt */
    if (t->tcr && (int)t->cti<=cti) {
      cti = t->cti;
#ifdef _DEBUG
      // $$$
      if ((int)t->cti < 0) {
        BREAKPOINT68;
      }
#endif
      itimer = t;
    }
  }
  return itimer;
}

/* Advance and update timer registers */
static void advance_timer(MFPtimer68_t *t, cycle68_t cycle)
{
  int c;
  unsigned int psc, tdr_mov;
  int tdr;

  /* Number of cycle to advance for update */
  c = cycle - t->udc;
  if (!c) {
    return;
  }

  if (!t->tcr) {
    /* Timer stopped, just stamp update-cycle */
    t->udc = cycle;
    return;
  }

#ifdef _DEBUG
  //$$$
  if (c < 0) {
    BREAKPOINT68;
  }
#endif
  t->udc = cycle;

  /* Advance prescaler */
  psc = t->psc_cnt + c;
  /* Count TDR decrements for this amount of prescale-cycle */
  tdr_mov = psc / t->psc_width;
  /* Remainder in prescaler */
  t->psc_cnt = psc % t->psc_width;

  tdr = t->tdr;
  tdr -= tdr_mov;
  if (tdr < 0) {
    /* $$$ TODO : check this :) */
    tdr = (tdr % -(int)t->tdr2) + t->tdr2;
  } else if (!tdr) {
    tdr = t->tdr2;
  }
  t->tdr = tdr;
}

/* Stop timer : tcr is set to zero */
static void stop_timer(MFPtimer68_t *t, cycle68_t cycle)
{
  if (!t->tcr) {
    /* No need to stop a stopped timer */
    return;
  }


#ifdef _DEBUG
  // $$$
  if ((int)t->cti < 0) {
    BREAKPOINT68;
  }
  if ((int)(t->cti + 191 - cycle) < 0) {
    BREAKPOINT68;
  }
#endif

//  BREAKPOINT68;
  /* Advance register to this cycle */
  advance_timer(t, cycle);

#ifdef _DEBUG
  // $$$
  if (t->tdr > 256) {
    BREAKPOINT68;
  }
#endif

  t->tcr = 0;
  t->psc_cnt = 0;
  t->psc_width = 0;
  t->cpp = 0;
}

/* Number of BOGO cycle before next interruption */
static unsigned int timer_cti(MFPtimer68_t *t)
{
#if _DEBUG
  if (t->psc_cnt >= t->psc_width || t->tdr == 0) {
    BREAKPOINT68;
  }
#endif
  return (t->tdr * t->psc_width) - t->psc_cnt;
}

/*  Adjust cycle to next interrupt
*/
static void timer_change_cr( MFPtimer68_t *t, cycle68_t cycle, u8 newtcr)
{
  /* Not really change so BYE BYE */
  if (newtcr == t->tcr) {
    return;
  }

#ifdef _DEBUG
  //$$$ This case is a little tricky : Changing timer prescale on the fly
  //    may have unpredictable result !!!
  // chipmon of synergy does !!!
  //BREAKPOINT68;
#endif

  advance_timer(t, cycle);
  t->psc_cnt = 0;

#ifdef _DEBUG
  // $$$ hack
  if ((int)t->cti < 0) {
    BREAKPOINT68;
  }
#endif

  t->tcr = newtcr;
  t->psc_width = f[t->tcr];
  t->cpp = t->tdr2 * t->psc_width ;
  t->cti = cycle + timer_cti(t);

#ifdef _DEBUG
  // $$$
  if ((int)t->cti < 0) {
    BREAKPOINT68;
  }
#endif
}


/* Resume timer : timer was stopped, */
static void resume_timer(MFPtimer68_t *t, u8 cr, cycle68_t cycle)
{
#ifdef _DEBUG
  if (t->tcr || t->psc_cnt) {
    BREAKPOINT68;
  }
#endif

  t->udc = cycle;
  t->tcr = cr;
  t->psc_width = f[t->tcr];
  t->cpp = t->tdr2 * t->psc_width;
  t->cti = cycle + timer_cti(t);

}

/* Timer loop with same parameters */
static void continue_timer(MFPtimer68_t *t, cycle68_t cycle)
{
//  advance_timer(t,cycle);
  t->cti += t->cpp;
#ifdef _DEBUG
  if (t->cti < cycle) {
    BREAKPOINT68;
  }
#endif
}

static void reset_timer(MFPtimer68_t *t)
{
  t->tcr = 0;
  t->tdr = t->tdr2 = 256;
  t->udc = t->cti = t->cpp = 0;
  t->psc_cnt = t->psc_width = 0;
}

/******************************************************
*                      MFP reset                      *
******************************************************/

int MFP_reset(void)
{
  int i;

  for (i=0; i<(int)sizeof(mfp); i++) {
    mfp[i]=0;
  }
  mfp[0x17] = 0x40;
  mfp[0x01] = 0x80;
  mfp_itimer = 0L;
  for (i=0; i<4;i++) {
    reset_timer(timers+i);
    mfp[0x1f + 2*i] = timers[i].tdr;
  }
  return 0;
}

/******************************************************
*                      MFP init                       *
******************************************************/

int MFP_init( void )
{
  MFP_reset();
  return 0;
}

/******************************************************
*              MFP get Timer Data register            *
******************************************************/

u8 MFP_getTDR(int timer, cycle68_t cycle)
{
  timer &= 3;
  if (timers[timer].tcr) {
    advance_timer(timers+timer, cycle * 192);
  }
  return timers[timer].tdr;
}

/******************************************************
*             MFP write Timer data register           *
******************************************************/

void MFP_putTDR(int timer, u8 v, cycle68_t cycle)
{
  cycle *= 192;
  timer &= 3;

  /* Interrupt when count down to 0, so 0 is 256 */
  timers[timer].tdr2 = v ? v : 0x100;

  /* If timer stopped, reload tdr */
  if (!timers[timer].tcr) {
    mfp[0x1f+2*timer] = timers[timer].tdr = timers[timer].tdr2;
  } else {
    timers[timer].cpp = timers[timer].tdr2 * timers[timer].psc_width;
  }
}

/******************************************************
*             MFP write Timer control register        *
******************************************************/

static void putTCR(MFPtimer68_t *t, u8 v, unsigned int cycle)
{
  if (!v) {
    stop_timer(t,cycle);
  } else if (!t->tcr) {
    resume_timer(t, v, cycle);
/*       unsigned int frq = 8000000*192 / t->cpp; */
  } else {
    timer_change_cr( t, cycle, v );
  }
}

void MFP_putTCR(int timer, u8 v, cycle68_t cycle)
{
  cycle *= 192;
  timer &= 3;

  if (timer < TIMER_C) {
    mfp[0x19+2*timer] = v;

#ifdef _DEBUG
    // $$$ Event mode + Pulse mode not emulate
    if (v&0x10) {
      BREAKPOINT68;
    }
#endif

    putTCR(timers+timer,v&7,cycle);
  }
  else
  {
    mfp[0x1D] = v;
    putTCR(timers+TIMER_C, (v>>4)&7, cycle);
    putTCR(timers+TIMER_D, v&7, cycle);
  }
  mfp_itimer = find_next_int();
}

/******************************************************
*         Is MFP generate an interruption ???         *
******************************************************/

int68_t *MFP_interrupt(cycle68_t cycle)
{
  static int68_t nxt_inter;
  MFPtimer68_t * cur_timer = mfp_itimer;

  /* No interrupt possible */
  if (!cur_timer) {
    return 0;
  }

  /* In BOGO please ! */
  cycle *= 192;
  /* $$$ Should add 191 ??? */

  /* Timer reach end */
#ifdef _DEBUG
  // $$$
  if ((int)cur_timer->cti < 0) {
    BREAKPOINT68;
  }
  if (cur_timer->tcr == 0) {
    BREAKPOINT68;
  }
#endif

  if ((int)cycle >= (int)cur_timer->cti) {
    continue_timer(cur_timer, cycle);
    mfp_itimer = find_next_int();

#ifndef _MFPIO68_SUPER_EMUL_
    /* simple Version */
    if ( mfp[0x07+cur_timer->channel] & mfp[0x13+cur_timer->channel] & cur_timer->bit )
    {
      nxt_inter.vector = MFP_VECTOR_BASE + cur_timer->vector;
      nxt_inter.level = cur_timer->level;
      return &nxt_inter;
    }
#ifdef _DEBUG
    else {
      // $$$ testing if this happen
      // $$$ This happen with all SSD tunes !
      //BREAKPOINT68;

    }
#endif

#else
    /* Test Interrupt Enable */
    if ( mfp[0x07+cur_timer->channel] & cur_timer->bit )
    {
      /* Set Interrupt Pending Bit */
      mfp[0x0B+cur_timer->channel] |= cur_timer->bit;

      /* Test Interrupt In Service */
      if (!(mfp[0x0F+cur_timer->channel]&cur_timer->bit))
      {
        /* SEI : Set Interrupt In Service */
        if (SEI) mfp[0x0B+cur_timer->channel] |= cur_timer->bit;
        /*  Test Interrupt Mask */
        if (mfp[0x13+cur_timer->channel] & cur_timer->bit)
        {
          nxt_inter.vector = MFP_VECTOR_BASE + cur_timer->vector;
          nxt_inter.level = cur_timer->level;
          return &nxt_inter;
        }
      }
    }

#endif


  }
#ifdef _DEBUG
  else {
    /* $$$ Should not happen ! */
    BREAKPOINT68;  /* MFP_interrupt() call but no int !! */
  }
#endif
  return 0;
}

/******************************************************
*         When MFP generates an interruption ???      *
******************************************************/
cycle68_t MFP_nextinterrupt(cycle68_t cycle)
{
  int v;
  if (mfp_itimer==0L) return IO68_NO_INT;
  v = (mfp_itimer->cti+191) / 192 - cycle;

#ifdef _DEBUG
  if (v < 0) {
    BREAKPOINT68; /* MFP_nextinterrupt() minus cycles */
  }
#endif
  return v;
}

/***************************************
*         change cycle count base      *
***************************************/
void MFP_subcycle(cycle68_t subcycle)
{
  int i;

  if (!subcycle) {
    return;
  }

  subcycle *= 192;
  for (i=0; i<4; ++i) {
    if (!timers[i].tcr) {
      continue;
    }

    timers[i].cti -= subcycle;

    if (timers[i].udc >= subcycle) {
      timers[i].udc -= subcycle;
    } else {
      advance_timer(timers+i, subcycle);
      timers[i].udc = 0;
    }

#ifdef _DEBUG
//    if ((int)timers[i].cti<0) BREAKPOINT68;
//    if ((int)timers[i].udc<0) BREAKPOINT68;
#endif
  // $$$ hack
    if ((int)timers[i].cti<0) timers[i].cti = 0;
    if ((int)timers[i].udc<0) timers[i].udc = 0;
  }
}
