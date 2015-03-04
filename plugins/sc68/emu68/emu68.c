/*
 *                             emu68 - api
 *         Copyright (C) 2001 Benjamin Gerard <ben@sashipa.com>
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

/* Copyright (C) 1998-2001 Ben(jamin) Gerard */

#include <config68.h>

#include <string.h>
                     
#include "emu68/emu68.h"

/*************************************************************
* Pointer to unique interruping IO (MFP for ST emulation!!!) *
*************************************************************/
io68_t *interrupt_io = 0;

/***************************
* 68000 internal registers *
***************************/
reg68_t reg68;

/************
 * Historic *
 ***********/
#ifdef _DEBUG
static u32 historic68[1024], *h68=historic68;
static unsigned int count_pass_68 = 0;
#endif

extern linefunc68_t *line_func[1024];

/*******************************
* EMU68 internal struct access *
*******************************/

/*  Set new interrupt IO
 *  return previous pointer
*/
io68_t *EMU68_set_interrupt_io(io68_t * io)
{
  io68_t * oldio = interrupt_io;
  interrupt_io = io;
  return oldio;
}

void EMU68_set_registers(const reg68_t *r)
{
  int i;

  if (!r) {
    return;
  }
  reg68.usp = r->usp;
  reg68.pc  = r->pc;
  reg68.sr  = r->sr;
  for(i=0; i<16; i++) {
    reg68.d[i] = r->d[i];
  }
}

void EMU68_get_registers(reg68_t *r)
{
  int i;

  if (!r) {
    return;
  }
  r->usp = reg68.usp;
  r->pc  = reg68.pc;
  r->sr  = reg68.sr;
  for(i=0; i<16; i++) {
    r->d[i] = reg68.d[i];
  }
}

void EMU68_set_cycle(cycle68_t cycle)
{
  reg68.cycle = cycle;
}

cycle68_t EMU68_get_cycle(void)
{
  return reg68.cycle;
}

/*******************************
* EMU68 on board memory access *
*******************************/
int EMU68_memvalid(u32 dest, unsigned int sz)
{
  if (sz > reg68.memsz) {
    return EMU68error_add("Not enought 68K memory ($%X>$%X)",sz,reg68.memsz);
  }
  sz = (dest + sz) & reg68.memmsk;
  if (sz < dest) {
    return EMU68error_add("68K memory overflow :($%X-$%X)",dest,sz);
  }
  return 0;
}

/* Peek & Poke
*/
u8 EMU68_peek(u32 addr)
{
  return reg68.mem[addr&reg68.memmsk];
}

u8 EMU68_poke(u32 addr, u8 v)
{
  return reg68.mem[addr&reg68.memmsk] = v;
}

/*  Write memory block to 68K on board memory
 */
int EMU68_memput(u32 dest, u8 *src, unsigned int sz)
{
  int err;

  if (err=EMU68_memvalid(dest,sz), err<0) {
    return err;
  }
  memcpy(reg68.mem+dest,src,sz);
  return 0;
}

/*  Read memory block from 68K on board memory
*/
int EMU68_memget(u8 *dest, u32 src, unsigned int sz)
{
  int err;

  if (err=EMU68_memvalid(src,sz), err<0) {
    return err;
  }
  memcpy(dest,reg68.mem+src,sz);
  return 0;
}

/* 68000 Hardware Reset
 * - PC = 0
 * - SR = 2700
 * - A7 = end of mem - 4
 * - All registers cleared
 * - All IO reseted
*/
void EMU68_reset(void)
{
  io68_t *io;

  for (io=reg68.iohead; io; io=io->next) {
    (io->reset)();
  }
  memset(reg68.d,0,sizeof(reg68.d)*2);
  reg68.cycle  = 0;
  reg68.pc     = 0;
  reg68.sr     = 0x2700;
  reg68.a[7]   = reg68.memsz-4;
  reg68.status = 0;

#ifdef _DEBUG
  for (h68=historic68;
       h68<historic68+(sizeof(historic68)/sizeof(*historic68)); h68++) {
    *h68 = 0x12345678;
  }
  count_pass_68=0;
#endif
}

/* 68000 OP-WORD format :
 * 1111 0000 0000 0000 ( LINE  )
 * 0000 0001 1111 1000 ( OP    )
 * 0000 0000 0000 0111 ( REG 0 )
 * 0000 1110 0000 0000 ( REG 9 )
*/

/* Emulator 68000 : Step 1 instruction
*/
void EMU68_step(void)
{
  int line,opw,reg9;

#ifdef _DEBUG

  /* Fill historic buffer */
  if (h68 == historic68) {
    h68 = historic68 + (sizeof(historic68)/sizeof(*historic68));
  }
  --h68;
  *h68 = reg68.pc;
  opw = (u16)get_nextw();

#else

  u8 * mem = reg68.mem + (reg68.pc & MEM68MSK);
  reg68.pc += 2;
  opw  = (mem[0]<<8) | mem[1];

#endif

  line = opw & 0xF000;
  opw -= line;
  reg9 = opw & 0x0E00;
  opw -= reg9;
  line |= opw<<3;
  reg9 >>= 9;
  line >>= 6;
  opw  &=  7;
  (line_func[line])(reg9, opw);
}

static void poll_rte(u32 stack)
{
  do
  {
    EMU68_step();
  } while( (u32) reg68.a[7] < stack);
}

/* Run until RTS level 0 occurs
 * and play Interruption for 1 pass.
 * ( Specific SC68 !!! )
*/
void EMU68_level_and_interrupt(cycle68_t cycleperpass)
{
  const u32 stack = reg68.a[7];
  u32 pc=reg68.pc, cycle, fd;
  io68_t *io;
  int68_t *t;

  reg68.a[7] = stack - 4;
#if _DEBUG
  EMU68_poke(reg68.a[7]+0, 0x12);
  EMU68_poke(reg68.a[7]+1, 0x34);
  EMU68_poke(reg68.a[7]+2, 0x56);
  EMU68_poke(reg68.a[7]+3, 0x78);
#endif

  /* Clear ORed memory access flags ... */
#ifdef EMU68DEBUG
  reg68.framechk = 0;
#endif

#if _DEBUG
  if (!count_pass_68) {
    if (cycleperpass) {
      BREAKPOINT68;
    }
    if (reg68.cycle) {
      BREAKPOINT68;
    }
  } else {
    if (!cycleperpass) {
      BREAKPOINT68;
    }
    if (count_pass_68 == 1) {
      if (reg68.cycle) {
        BREAKPOINT68;
      }
    } else {
      if (!reg68.cycle) {
        BREAKPOINT68;
      }
    }

  }
  count_pass_68++;
#endif

  /* Adjust internal IO cycle counter & find MFP */
  for (io=reg68.iohead; io; io=io->next) {
    (io->adjust_cycle)(reg68.cycle);
  }
  reg68.cycle = 0;

  /* Do until RTS */
  do {
    EMU68_step();
  } while (stack > (u32)reg68.a[7]);

  cycle=0;

#ifdef _DEBUG
  if (stack != (u32)reg68.a[7]) {
    BREAKPOINT68;
  }
  if (reg68.pc != 0x12345678) {
    BREAKPOINT68;
  }
#endif

  /* Get interrupt IO (MFP) if any */
  if (interrupt_io) {
    int ipl = (reg68.sr & (7<<SR_IPL_BIT)) >> SR_IPL_BIT;
#ifdef _DEBUG
    int ipl_lost = 0;
    int irq_pass = 0;
#endif
    /* $$$ HACK : mfp interupt at level 5 */
    if ( 6 > ipl ) {
      /* Spool interrupt */
      while (fd=(interrupt_io->next_int)(cycle), fd != IO68_NO_INT)
      {
#ifdef _DEBUG
        irq_pass++;
#endif
        /* ... This must not happen ...  */
        if( (int)fd < 0 ) {
#ifdef _DEBUG
          BREAKPOINT68;
#endif
          fd = 0;
        }

        /* ... */

        /* TIME travel to next interruption */
        cycle += fd;

        if(cycle >= cycleperpass) {
          /* interrupt after this pass */
          break;
        }

        /* Execute mfp interrupt emulation */
        t = (interrupt_io->interrupt)(cycle);
        if (t) {
          reg68.cycle=cycle;
          reg68.a[7] = stack; /* $$$ Safety net */
          reg68.pc = 0x12345678;
          EXCEPTION(t->vector,t->level);
          poll_rte(stack);
#ifdef _DEBUG
          if (reg68.pc != 0x12345678) {
            BREAKPOINT68;
          }
          if (reg68.a[7] != stack) {
            BREAKPOINT68;
          }
          if (!(reg68.sr & 0x2000)) {
            BREAKPOINT68;
          }
#endif
        } else {
          //          BREAKPOINT68;
        }
      }
    } else {
      // ipl masks irq
#ifdef _DEBUG
      BREAKPOINT68
      ipl_lost++;
#endif
    }
  }

  /* Restore A7, PC, and new cycle counter */
  reg68.a[7] = stack;
  reg68.pc = pc;
  reg68.cycle = cycleperpass;
}

/* Run emulation for given number of cycle
 */
void EMU68_cycle(cycle68_t cycleperpass)
{
  cycle68_t cycle;
  io68_t *io;

  /* Clear ORed memory access flags ... */
#ifdef EMU68DEBUG
  reg68.framechk = 0;
#endif

  /* Adjust internal IO cycle counter */
  for(io=reg68.iohead; io; io=io->next) {
    (io->adjust_cycle)(reg68.cycle);
  }

  /* Do until RTS */
  cycle = reg68.cycle = 0;
  do
  {
    /* If not stopped, execute an instruction */
    if (!reg68.status) {
      EMU68_step();
      cycle += 4;
    }
    reg68.cycle = cycle;

    if (interrupt_io) {
      int68_t *t;
      int ipl;

      ipl = (reg68.sr & (7<<SR_IPL_BIT)) >> SR_IPL_BIT;

      /* $$$ HACK : mfp interupt at level 5 */
      if (6 > ipl) {
	t = (interrupt_io->interrupt)(cycle);
	if (t) {
	  reg68.status = 0;
	  EXCEPTION(t->vector,t->level);
	  /* $$$ cycle should be added here for exception handle */
	}
      }
    }
  } while(cycle < cycleperpass);

  // $$$ This is hack : should be "cycle" and handle later
  cycle = cycleperpass;
  reg68.cycle = cycle;
}


/* Run until PC = breakpc
*/
void EMU68_break (u32 breakpc)
{
  while( (u32) reg68.pc != breakpc) {
    EMU68_step();
  }
}

/* EMU68 fisrt time init
 * maxmem : 68000 memory requested in byte ( power of 2 only !!! )
 *
 * returns 0=NO ERROR
*/
int EMU68_init(u8 * buffer, u32 maxmem)
{
  int i;

  /* Clear 68000 registers and everything. */
  memset(&reg68,0,sizeof(reg68));
  interrupt_io = 0;

  if (!buffer) {
    return EMU68error_add("EMU68_init : NULL pointer");
  }

  /* Check memory bound 128Kb <= MEM REQUESTED <= 16MB */
  if (maxmem < (1<<17)) {
    return EMU68error_add("EMU68_init : memory amount must be >= 128Kb");
  } else if (maxmem > (1<<24)) {
    return EMU68error_add("EMU68_init : memory amount must be < 16mb");
  }
  for (i=0; i<24 && maxmem!=((unsigned)1<<i); i++)
    ;
  if (maxmem != ((unsigned)1<<i)) {
    return EMU68error_add("EMU68_init : memory amount must be a power of 2");
  }

  /* Must be done before next test */
  reg68.mem = buffer;
  reg68.memsz = maxmem;
  reg68.memmsk = maxmem-1;

  /*  This test is neccessary becoz of possible optimizations in
   *  EMU68 compilation !!!
   */
  if(MEM68MSK != maxmem-1) {
    return EMU68error_add( "EMU68_init : memory amount incompatible"
                           " with EMU68 version");
  }

  /* Set 68000 memory buffer */
#ifndef EMU68DEBUG  
  memset(reg68.mem,0,maxmem+3);
#else
  memset(reg68.mem,0,maxmem << 1);
  reg68.chk = reg68.mem + maxmem;
#endif
  
  EMU68memory_init();
  EMU68_reset();
  return 0;
}

/** Clean exit.
 */
void EMU68_kill(void)
{
  interrupt_io = 0;
  memset(&reg68,0,sizeof(reg68));
}

/* Call & check linkage error !!!
*/
int EMU68_debugmode(void)
{
#ifdef EMU68DEBUG
  return 1;
#else
  return 0;
#endif
}
