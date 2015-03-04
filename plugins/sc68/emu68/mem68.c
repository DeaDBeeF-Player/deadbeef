/**
 * @ingroup   emu68_devel
 * @file      mem68.c
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @date      1999/03/13
 * @brief     68000 memory and io access
 * @version   $Id: mem68.c,v 2.0 2003/08/21 04:58:35 benjihan Exp $
 */

/* Copyright (C) 1998-2001 Ben(jamin) Gerard */

#include "emu68/mem68.h"

extern reg68_t reg68;

/*  Memory quick acces tables
 */
memrfunc68_t read_mem_jmp_l[256];
memrfunc68_t read_mem_jmp_w[256];
memrfunc68_t read_mem_jmp_b[256];
memwfunc68_t write_mem_jmp_l[256];
memwfunc68_t write_mem_jmp_w[256];
memwfunc68_t write_mem_jmp_b[256];

/* Dummy IO acces functions
 */
static u32 read_mem_dummy(u32 addr, u32 cycle)
{
  addr=addr; cycle=cycle;
  return 0;
}

static void write_mem_dummy(u32 addr, u32 v, u32 cycle)
{
  addr=addr; v=v; cycle=cycle;
}

/* Set memory access check flags, and store ORed frame global check
 * for music time calculation ( for SC68 ! )
*/
#ifdef EMU68DEBUG
static void chkframe( u32 addr, u8 flags )
{
  u8 oldchk = reg68.chk[addr & reg68.memmsk];
  if((oldchk&flags) != flags)
  {
    reg68.framechk |= flags;
    reg68.chk[addr] = oldchk|flags;
  }
}
#endif

/*
 * 68000 memory acces functions
 *
 * This functions don't check odd address and privilege violation
 */

u32 read_68000mem_b(u32 addr)
{
  if(ISIO68(addr))
    return (read_mem_jmp_b[((addr)>>8)&255])(addr,reg68.cycle);
  addr &= MEM68MSK;
#ifdef EMU68DEBUG
  chkframe(addr,READ_68);
#endif
  return reg68.mem[addr];
}

void write_68000mem_b(u32 addr, u32 v)
{
  if(ISIO68(addr))
  {
    (write_mem_jmp_b[((addr)>>8)&255])(addr,v,reg68.cycle);
    return;
  }
  addr &= MEM68MSK;
#ifdef EMU68DEBUG
  chkframe(addr,WRITTEN_68);
#endif
  reg68.mem[addr] = v;
}

u32 read_68000mem_w(u32 addr)
{
  if(ISIO68(addr))
    return (read_mem_jmp_w[((addr)>>8)&255])(addr,reg68.cycle);
  else
  {
#ifndef EMU68DEBUG
    u8 *mem = reg68.mem+(addr&MEM68MSK);
    return (mem[0]<<8) +
            mem[1];
#else
  return (read_68000mem_b(addr+0)<<8) +
          read_68000mem_b(addr+1);
#endif
  }
}

void write_68000mem_w(u32 addr, u32 v)
{
  if(ISIO68(addr))
  {
    (write_mem_jmp_w[((addr)>>8)&255])(addr,v,reg68.cycle);
  }
  else
  {
#ifndef EMU68DEBUG
    u8 *mem = reg68.mem+(addr&MEM68MSK);
    mem[0] = v>>8;
    mem[1] = v;
#else
    write_68000mem_b(addr+0,v>>8);
    write_68000mem_b(addr+1,v);
#endif
  }
}

u32 read_68000mem_l(u32 addr)
{
  if(ISIO68(addr))
    return (read_mem_jmp_l[((addr)>>8)&255])(addr,reg68.cycle);
  else
  {
#ifndef EMU68DEBUG
    u8 *mem = reg68.mem+(addr&MEM68MSK);
    return (mem[0]<<24) +
           (mem[1]<<16) +
           (mem[2]<<8) +
            mem[3];
#else
    return (read_68000mem_b(addr+0)<<24) +
           (read_68000mem_b(addr+1)<<16) +
           (read_68000mem_b(addr+2)<<8) +
            read_68000mem_b(addr+3);
#endif
  }
}

void write_68000mem_l(u32 addr, u32 v)
{
  if(ISIO68(addr))
  {
    (write_mem_jmp_l[((addr)>>8)&255])(addr,v,reg68.cycle);
    return;
  }
  else
  {
#ifndef EMU68DEBUG
    u8 *mem = reg68.mem+(addr&MEM68MSK);
    mem[0] = v>>24;
    mem[1] = v>>16;
    mem[2] = v>>8;
    mem[3] = v;
#else
    write_68000mem_b(addr+0,v>>24);
    write_68000mem_b(addr+1,v>>16);
    write_68000mem_b(addr+2,v>>8);
    write_68000mem_b(addr+3,v);
#endif
  }
}

/* Read 68000 (PC)+ word
 * - This version assume PC is in 68000 memory
 *   as long I don't try to make 68000 execute
 *   @ IO address, I assume it is not possible !
 */
s32 get_nextw( void )
{
  s32 v;
  u32 pc  = reg68.pc;
#ifndef EMU68DEBUG
  u8 *mem = reg68.mem+(pc&MEM68MSK);
  v =  (s32)(s8)mem[0]<<8;
  v |=          mem[1];
#else
  u32 adr = pc&MEM68MSK;
  chkframe(adr,READ_68+EXECUTED_68);
  v               = (s32)(s8)reg68.mem[adr]<<8;
  adr = (adr+1)&MEM68MSK;
  chkframe(adr,READ_68+EXECUTED_68);
  v              |= reg68.mem[adr];
#endif
  reg68.pc = pc+2;
  return v;
}

/* Read 68000 (PC)+ long
* See : get_nextw()
*/
s32 get_nextl( void )
{
  s32 v;
#ifndef EMU68DEBUG
  u32 pc  = reg68.pc;
  u8 *mem = reg68.mem+(pc&MEM68MSK);
  v =  (s32)(s8)mem[0]<<24;
  v |=          mem[1]<<16;
  v |=          mem[2]<<8;
  v |=          mem[3];
  reg68.pc = pc+4;
#else
  v  = get_nextw()<<16;
  v |= (u16)get_nextw();
#endif
  return v;
}

void pushw( s32 v )
{
  reg68.a[7]-=2;
  write_W(reg68.a[7],v);
}

void pushl( s32 v )
{
  reg68.a[7]-=4;
  write_L(reg68.a[7],v);
}

s32 popl(void)
{
  s32 v = read_L(reg68.a[7]);
  reg68.a[7] += 4;
  return v;
}

s32 popw(void)
{
  s32 v = read_W(reg68.a[7]);
  reg68.a[7] += 2;
  return v;
}

/* Init memory quick acces table for SC68
*/
void EMU68memory_init( void )
{
  EMU68memory_reset();
}

/* Reset memory quick acces table for SC68
*/
void EMU68memory_reset( void )
{
  int i;
  for(i=0; i<256; i++)
    EMU68memory_reset_area((u8)i);
}

/*  Add a new memory acces control area ( for new io )
*/
void EMU68memory_new_area( u8 area,
                          memrfunc68_t *read_bwl,
                          memwfunc68_t *write_bwl)
{
  read_mem_jmp_b[area] = read_bwl[0]!=NULL? read_bwl[0] : read_mem_dummy;
  read_mem_jmp_w[area] = read_bwl[1]!=NULL? read_bwl[1] : read_mem_dummy;
  read_mem_jmp_l[area] = read_bwl[2]!=NULL? read_bwl[2] : read_mem_dummy;
  write_mem_jmp_b[area] = write_bwl[0]!=NULL? write_bwl[0] : write_mem_dummy;
  write_mem_jmp_w[area] = write_bwl[1]!=NULL? write_bwl[1] : write_mem_dummy;
  write_mem_jmp_l[area] = write_bwl[2]!=NULL? write_bwl[2] : write_mem_dummy;
}

/*  Reset memory acces control area to normal state :
 *   areas:
 *     [00-7F] => memory
 *     [80-FF] => dummy IO
*/
void EMU68memory_reset_area( u8 area )
{
  read_mem_jmp_l[area] =
  read_mem_jmp_w[area] =
  read_mem_jmp_b[area] = read_mem_dummy;
  write_mem_jmp_l[area] =
  write_mem_jmp_w[area] =
  write_mem_jmp_b[area] = write_mem_dummy;
}
